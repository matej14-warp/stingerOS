/* scorpion OS - kernel/smp.c
   APIC-based SMP support.

   ACPI MADT parsing:
   - Walk RSDT/XSDT looking for MADT (signature "APIC")
   - Parse processor local APIC entries (type 0) to enumerate CPUs
   - Parse I/O APIC entries (type 1) for future use

   AP wakeup sequence (Intel MP spec):
   1. BSP writes trampoline code to 4KB page at low physical address
   2. BSP sends INIT IPI to target APIC ID
   3. BSP waits 10ms
   4. BSP sends SIPI IPI with vector = page >> 12
   5. AP starts in real mode at vector * 0x1000, sets up protected mode, calls smp_ap_entry()
   6. AP parks in halt loop, sets online=1

   On QEMU with -smp 2 this actually works.                          */

#include "smp.h"
#include "terminal.h"
#include <stdint.h>
#include <stddef.h>
#include "serial_debug.h"

cpu_t g_cpus[SMP_MAX_CPUS];
int   g_cpu_count = 0;

/* AP work-queue: BSP sets these then sends IPI 0x40 */
volatile ap_work_fn_t  g_ap_work_fn  = 0;
void * volatile        g_ap_work_arg = 0;
volatile int    g_ap_work_done        = 1;   /* 1 = idle */

/* ---- Spinlock delay ---- */
static void delay_10ms(void) {
    for (volatile uint32_t i = 0; i < 500000; i++);
}

/* ---- LAPIC MMIO helpers ---- */
static volatile uint32_t *lapic = (volatile uint32_t *)LAPIC_BASE;

static inline uint32_t lapic_read(uint32_t reg) {
    return lapic[reg >> 2];
}
static inline void lapic_write(uint32_t reg, uint32_t val) {
    lapic[reg >> 2] = val;
}

uint8_t lapic_get_id(void) {
    return (uint8_t)((lapic_read(LAPIC_ID) >> 24) & 0xFF);
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}

void lapic_init(void) {
    /* Enable LAPIC: set SVR bit 8 */
    uint32_t svr = lapic_read(LAPIC_SVR);
    lapic_write(LAPIC_SVR, svr | 0x100 | 0xFF);   /* enable + spurious vector 0xFF */
    terminal_printf("[smp] local APIC id=%u enabled\n", lapic_get_id());
    serial_printf("[smp] lapic_init: id=%u SVR=%x\n", lapic_get_id(), lapic_read(LAPIC_SVR));
}

void lapic_send_ipi(uint8_t apic_id, uint32_t icr_lo) {
    lapic_write(LAPIC_ICR_HI, (uint32_t)apic_id << 24);
    lapic_write(LAPIC_ICR_LO, icr_lo);
    uint32_t timeout = 100000;
    while ((lapic_read(LAPIC_ICR_LO) & (1 << 12)) && --timeout);
}

/* ---- ACPI MADT parsing ---- */
#define RSDP_SEARCH_START  0x000E0000
#define RSDP_SEARCH_END    0x000FFFFF
#define RSDP_SIG           "RSD PTR "

typedef struct {
    char     signature[8];
    uint8_t  checksum;
    char     oem[6];
    uint8_t  revision;
    uint32_t rsdt_addr;
} __attribute__((packed)) rsdp_t;

typedef struct {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_rev;
} __attribute__((packed)) acpi_sdt_hdr_t;

typedef struct {
    acpi_sdt_hdr_t hdr;
    uint32_t       lapic_addr;
    uint32_t       flags;
} __attribute__((packed)) madt_t;

#define MADT_TYPE_LAPIC   0
#define MADT_TYPE_IOAPIC  1

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t acpi_proc_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_t;

static rsdp_t *find_rsdp(void) {
    for (uint32_t addr = RSDP_SEARCH_START; addr < RSDP_SEARCH_END; addr += 16) {
        if (*(uint64_t *)addr == *(uint64_t *)RSDP_SIG) {
            uint8_t sum = 0;
            for (int i = 0; i < 20; i++) sum += ((uint8_t *)addr)[i];
            if (sum == 0) return (rsdp_t *)addr;
        }
    }
    return NULL;
}

static acpi_sdt_hdr_t *find_table(uint32_t rsdt_addr, const char *sig) {
    acpi_sdt_hdr_t *rsdt = (acpi_sdt_hdr_t *)rsdt_addr;
    uint32_t *entries = (uint32_t *)((uint8_t *)rsdt + sizeof(acpi_sdt_hdr_t));
    uint32_t count = (rsdt->length - sizeof(acpi_sdt_hdr_t)) / 4;
    for (uint32_t i = 0; i < count; i++) {
        acpi_sdt_hdr_t *tbl = (acpi_sdt_hdr_t *)entries[i];
        if (tbl->signature[0]==sig[0] && tbl->signature[1]==sig[1] &&
            tbl->signature[2]==sig[2] && tbl->signature[3]==sig[3])
            return tbl;
    }
    return NULL;
}

int smp_detect(void) {
    g_cpu_count = 0;
    g_cpus[0].apic_id = lapic_get_id();
    g_cpus[0].is_bsp  = 1;
    g_cpus[0].online  = 1;
    g_cpus[0].core_id = 0;
    g_cpu_count = 1;

    rsdp_t *rsdp = find_rsdp();
    if (!rsdp) {
        terminal_printf("[smp] RSDP not found — SMP not available\n");
        return g_cpu_count;
    }
    terminal_printf("[smp] RSDP found at %x\n", (uint32_t)rsdp);
    serial_printf("[smp] RSDP found at %x revision=%u rsdt=%x\n", (uint32_t)rsdp, rsdp->revision, rsdp->rsdt_addr);

    acpi_sdt_hdr_t *madt_hdr = find_table(rsdp->rsdt_addr, "APIC");
    if (!madt_hdr) {
        terminal_printf("[smp] MADT not found — single CPU assumed\n");
        return g_cpu_count;
    }

    madt_t *madt = (madt_t *)madt_hdr;
    uint8_t *ptr = (uint8_t *)madt + sizeof(madt_t);
    uint8_t *end = (uint8_t *)madt + madt->hdr.length;

    while (ptr < end) {
        uint8_t type = ptr[0];
        uint8_t len  = ptr[1];
        if (len < 2) break;
        if (type == MADT_TYPE_LAPIC) {
            madt_lapic_t *entry = (madt_lapic_t *)ptr;
            if ((entry->flags & 1) && entry->apic_id != g_cpus[0].apic_id) {
                if (g_cpu_count < SMP_MAX_CPUS) {
                    g_cpus[g_cpu_count].apic_id = entry->apic_id;
                    g_cpus[g_cpu_count].is_bsp  = 0;
                    g_cpus[g_cpu_count].online  = 0;
                    g_cpus[g_cpu_count].core_id = (uint32_t)g_cpu_count;
                    g_cpu_count++;
                }
            }
        }
        ptr += len;
    }

    terminal_printf("[smp] detected %d CPU(s) via MADT\n", g_cpu_count);
    return g_cpu_count;
}

#define AP_STACK_SIZE 4096
static uint8_t ap_stack[AP_STACK_SIZE] __attribute__((aligned(16)));

static void write_ap_trampoline(uint32_t paddr) {
    uint8_t *p = (uint8_t *)paddr;
    for (int i = 0; i < 4096; i++) p[i] = 0;

    p[0x00] = 0xEA;
    p[0x01] = 0x40; p[0x02] = 0x80;
    p[0x03] = 0x00; p[0x04] = 0x00;

    uint8_t gdt_tbl[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0xFF,0xFF,0x00,0x00,0x00,0x9A,0xCF,0x00,
        0xFF,0xFF,0x00,0x00,0x00,0x92,0xCF,0x00,
    };
    for (int i = 0; i < (int)sizeof(gdt_tbl); i++) p[0x10 + i] = gdt_tbl[i];

    uint32_t gdt_base = paddr + 0x10;
    p[0x28] = 23; p[0x29] = 0;
    p[0x2A] = (uint8_t)(gdt_base);
    p[0x2B] = (uint8_t)(gdt_base >> 8);
    p[0x2C] = (uint8_t)(gdt_base >> 16);
    p[0x2D] = (uint8_t)(gdt_base >> 24);

    uint32_t entry_ptr = (uint32_t)(uintptr_t)smp_ap_entry;
    p[0x34] = (uint8_t)(entry_ptr);
    p[0x35] = (uint8_t)(entry_ptr >> 8);
    p[0x36] = (uint8_t)(entry_ptr >> 16);
    p[0x37] = (uint8_t)(entry_ptr >> 24);

    uint32_t stack_top = (uint32_t)(uintptr_t)(ap_stack + AP_STACK_SIZE);
    p[0x38] = (uint8_t)(stack_top);
    p[0x39] = (uint8_t)(stack_top >> 8);
    p[0x3A] = (uint8_t)(stack_top >> 16);
    p[0x3B] = (uint8_t)(stack_top >> 24);

    uint8_t code16[] = {
        0xBA, 0xF8, 0x03,
        0xB0, 0x40, 0xEE,
        0xFA,
        0xFC,
        0xB0, 0x41, 0xEE,
        0x31, 0xC0,
        0x8E, 0xD8,
        0xB0, 0x42, 0xEE,
        0x0F, 0x01, 0x16, 0x28, 0x80,
        0xB0, 0x43, 0xEE,
        0x0F, 0x20, 0xC0,
        0x0C, 0x01,
        0x0F, 0x22, 0xC0,
        0xB0, 0x44, 0xEE,
        0xEA,
        0x80, 0x80,
        0x08, 0x00,
    };
    for (int i = 0; i < (int)sizeof(code16); i++) p[0x40 + i] = code16[i];

    uint8_t code32[] = {
        0xB0, 0x45, 0xEE,
        0xB8, 0x10, 0x00, 0x00, 0x00,
        0x8E, 0xD8,
        0x8E, 0xC0,
        0x8E, 0xE0,
        0x8E, 0xE8,
        0x8E, 0xD0,
        0xBA, 0xF8, 0x03, 0x00, 0x00,
        0xB0, 0x46, 0xEE,
        0x8B, 0x25,
        (uint8_t)((paddr + 0x38)),
        (uint8_t)((paddr + 0x38) >> 8),
        (uint8_t)((paddr + 0x38) >> 16),
        (uint8_t)((paddr + 0x38) >> 24),
        0xB0, 0x47, 0xEE,
        0xFF, 0x15,
        (uint8_t)((paddr + 0x34)),
        (uint8_t)((paddr + 0x34) >> 8),
        (uint8_t)((paddr + 0x34) >> 16),
        (uint8_t)((paddr + 0x34) >> 24),
        0xF4,
        0xEB, 0xFE,
    };
    for (int i = 0; i < (int)sizeof(code32); i++) p[0x80 + i] = code32[i];
}

int smp_boot_ap(uint8_t apic_id, uint32_t trampoline_page) {
    uint32_t paddr = trampoline_page << 12;
    write_ap_trampoline(paddr);
    lapic_send_ipi(apic_id, IPI_INIT | (1 << 14) | (1 << 15));
    delay_10ms(); delay_10ms();
    lapic_send_ipi(apic_id, IPI_INIT | (1 << 15));
    delay_10ms(); delay_10ms();
    for (int i = 0; i < 2; i++) {
        lapic_send_ipi(apic_id, IPI_SIPI | trampoline_page);
        for (volatile int w = 0; w < 1000000; w++);
        delay_10ms();
    }
    uint32_t timeout = 1000000;
    for (int c = 0; c < g_cpu_count; c++) {
        if (g_cpus[c].apic_id == apic_id) {
            while (!g_cpus[c].online && --timeout);
            return g_cpus[c].online ? 0 : -1;
        }
    }
    return -1;
}

void smp_print_status(void) {
    terminal_printf("CPU  APIC  Role  Status\n");
    terminal_printf("---- ----  ----  ------\n");
    for (int i = 0; i < g_cpu_count; i++) {
        terminal_printf("%-4d %-4u  %-4s  %s\n",
            i, g_cpus[i].apic_id,
            g_cpus[i].is_bsp ? "BSP" : "AP",
            g_cpus[i].online ? "online" : "offline");
    }
}

int smp_is_busy(void) {
    return !g_ap_work_done;
}

int smp_dispatch_ap(void (*fn)(void*), void *arg, int wait) {
    uint8_t ap_apic = 0;
    int found = 0;
    for (int i = 0; i < g_cpu_count; i++) {
        if (!g_cpus[i].is_bsp && g_cpus[i].online) {
            ap_apic = g_cpus[i].apic_id;
            found = 1;
            break;
        }
    }
    if (!found) return -1;
    
    if (!wait && !g_ap_work_done) return -1; /* Already busy, don't block BSP */
    
    while (!g_ap_work_done) __asm__ volatile ("pause");
    g_ap_work_done = 0;
    g_ap_work_arg  = arg;
    g_ap_work_fn   = fn;
    lapic_send_ipi(ap_apic, IPI_FIXED | 0x40);
    if (wait) {
        while (!g_ap_work_done) __asm__ volatile ("pause");
    }
    return 0;
}

void __attribute__((noreturn)) smp_ap_entry(void) {
    extern void ap_load_idt(void);
    ap_load_idt();
    lapic_init();
    uint8_t my_id = lapic_get_id();
    for (int i = 0; i < g_cpu_count; i++) {
        if (g_cpus[i].apic_id == my_id) {
            g_cpus[i].online = 1;
            break;
        }
    }
    __asm__ volatile ("sti");
    for (;;) {
        __asm__ volatile ("hlt");
        if (g_ap_work_fn) {
            void (*fn)(void*) = (void(*)(void*))g_ap_work_fn;
            void *arg         = (void *)g_ap_work_arg;
            g_ap_work_fn  = 0;
            g_ap_work_arg = 0;
            fn(arg);
            g_ap_work_done = 1;
        }
    }
}
