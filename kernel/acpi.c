/* scorpion OS - kernel/acpi.c
   ACPI init: find RSDP → FADT → PM1a control block + S5 sleep type.
   acpi_poweroff() writes SLP_EN to PM1a_CNT for QEMU/KVM shutdown.
   acpi_reboot()   writes 0x06 to port 0xCF9 (PCI reset) as fallback.
*/
#include "acpi.h"
#include "terminal.h"
#include <stdint.h>

uint16_t g_acpi_pm1a_cnt = 0;
uint16_t g_acpi_slp_typa = 0;
uint16_t g_acpi_slp_typb = 0;
int      g_acpi_ready    = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0,%1"::"a"(val),"Nd"(port));
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0,%1"::"a"(val),"Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port)); return v;
}

/* ---- RSDP search ---- */
typedef struct {
    char     sig[8];
    uint8_t  checksum;
    char     oem[6];
    uint8_t  revision;
    uint32_t rsdt_addr;
} __attribute__((packed)) rsdp_t;

typedef struct {
    char     sig[4];
    uint32_t length;
    uint8_t  rev, checksum;
    char     oem_id[6], oem_table[8];
    uint32_t oem_rev, creator_id, creator_rev;
} __attribute__((packed)) sdt_hdr_t;

typedef struct {
    sdt_hdr_t hdr;
    uint32_t  entries[1];
} __attribute__((packed)) rsdt_t;

/* FADT (simplified) */
typedef struct {
    sdt_hdr_t hdr;
    uint32_t  firmware_ctrl;
    uint32_t  dsdt;
    uint8_t   reserved0;
    uint8_t   preferred_pm_profile;
    uint16_t  sci_int;
    uint32_t  smi_cmd;
    uint8_t   acpi_enable;
    uint8_t   acpi_disable;
    uint8_t   s4bios_req;
    uint8_t   pstate_cnt;
    uint32_t  pm1a_evt_blk;
    uint32_t  pm1b_evt_blk;
    uint32_t  pm1a_cnt_blk;
    uint32_t  pm1b_cnt_blk;
} __attribute__((packed)) fadt_t;

static rsdp_t *find_rsdp(void) {
    for (uint32_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        rsdp_t *r = (rsdp_t *)addr;
        if (r->sig[0]=='R'&&r->sig[1]=='S'&&r->sig[2]=='D'&&r->sig[3]==' '&&
            r->sig[4]=='P'&&r->sig[5]=='T'&&r->sig[6]=='R'&&r->sig[7]==' ') {
            uint8_t sum = 0;
            for (int i=0;i<20;i++) sum+=((uint8_t*)r)[i];
            if (!sum) return r;
        }
    }
    return NULL;
}

static sdt_hdr_t *rsdt_find(rsdt_t *rsdt, const char *sig) {
    uint32_t n = (rsdt->hdr.length - sizeof(sdt_hdr_t)) / 4;
    for (uint32_t i=0;i<n;i++) {
        sdt_hdr_t *t = (sdt_hdr_t *)rsdt->entries[i];
        if (!t) continue;
        if (t->sig[0]==sig[0]&&t->sig[1]==sig[1]&&t->sig[2]==sig[2]&&t->sig[3]==sig[3])
            return t;
    }
    return NULL;
}

/* Walk AML in DSDT/SSDT looking for \_S5 package */
static int parse_s5(const uint8_t *aml, size_t len, uint16_t *slp_typa, uint16_t *slp_typb) {
    for (size_t i=0; i+5 < len; i++) {
        /* Look for "_S5_" signature */
        if (aml[i]=='_'&&aml[i+1]=='S'&&aml[i+2]=='5'&&aml[i+3]=='_') {
            /* Skip to Package opcode (0x12) */
            size_t j=i+4;
            if (j<len && aml[j]==0x12) j++;  /* Package */
            while(j<len && (aml[j]&0x40)) j++;  /* PkgLength */
            j++;
            if (j+2>=len) return -1;
            /* Two ByteConst: 0x0A val */
            if (aml[j]==0x0A) { *slp_typa = (uint16_t)(aml[j+1]<<10); j+=2; }
            else if (aml[j]==0x00) { *slp_typa = 0; j++; }
            if (j<len && aml[j]==0x0A) { *slp_typb = (uint16_t)(aml[j+1]<<10); }
            else *slp_typb = *slp_typa;
            return 0;
        }
    }
    return -1;
}

int acpi_init(void) {
    rsdp_t *rsdp = find_rsdp();
    if (!rsdp) return -1;

    rsdt_t *rsdt = (rsdt_t *)rsdp->rsdt_addr;
    if (!rsdt) return -1;

    sdt_hdr_t *fadt_hdr = rsdt_find(rsdt, "FACP");
    if (!fadt_hdr) return -1;

    fadt_t *fadt = (fadt_t *)fadt_hdr;
    g_acpi_pm1a_cnt = (uint16_t)fadt->pm1a_cnt_blk;

    /* Parse DSDT for _S5 */
    sdt_hdr_t *dsdt = (sdt_hdr_t *)fadt->dsdt;
    if (dsdt) {
        uint8_t *aml = (uint8_t *)dsdt + sizeof(sdt_hdr_t);
        size_t aml_len = dsdt->length - sizeof(sdt_hdr_t);
        parse_s5(aml, aml_len, &g_acpi_slp_typa, &g_acpi_slp_typb);
    }

    /* Enable ACPI via SMI_CMD if needed */
    if (fadt->smi_cmd && fadt->acpi_enable) {
        if (!(inb((uint16_t)fadt->pm1a_cnt_blk) & 1)) {
            outb((uint16_t)fadt->smi_cmd, fadt->acpi_enable);
            for (int i=0;i<1000;i++) {
                for (volatile int j=0;j<1000;j++);
                if (inb((uint16_t)fadt->pm1a_cnt_blk) & 1) break;
            }
        }
    }

    g_acpi_ready = 1;
    return 0;
}

void acpi_enable(void) { /* already done in acpi_init */ }

void acpi_poweroff(void) {
    if (g_acpi_ready && g_acpi_pm1a_cnt) {
        outw(g_acpi_pm1a_cnt, (uint16_t)(g_acpi_slp_typa | 0x2000));
    }
    /* QEMU fallback: write 'S' to debug exit port (isa-debug-exit) */
    outb(0x604, 0x02);  /* QEMU fw_cfg shutdown */
    /* Second fallback: APM */
    outb(0xB2, 0x00);
    __asm__ volatile("cli; hlt");
    for (;;) __asm__ volatile("hlt");
}

void acpi_reboot(void) {
    /* ACPI reset via reset register (most systems) */
    if (g_acpi_ready) {
        /* Try FADT reset register (offset 0x74) — not parsed above,
           fall through to PCI reset which always works on QEMU.    */
    }
    /* PCI reset: assert then deassert INIT# on port 0xCF9 */
    outb(0xCF9, 0x06);
    for (volatile int i=0;i<100000;i++);
    /* Keyboard controller pulse (legacy) */
    for (int i=0;i<0x10000;i++){
        if(!(inb(0x64)&2)) break;
    }
    outb(0x64, 0xFE);
    __asm__ volatile("cli; hlt");
    for (;;) __asm__ volatile("hlt");
}
