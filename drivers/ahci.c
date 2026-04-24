/* scorpion OS - drivers/ahci.c
   AHCI (Serial ATA) driver.
   Finds the AHCI HBA on PCI class 01h/subclass 06h, maps ABAR (BAR5),
   and issues 28/48-bit LBA PIO-via-AHCI commands using the command list
   and FIS receive structures in allocated memory.
   Supports QEMU AHCI (-device ich9-ahci or -device ahci,id=ahci).    */

#include "ahci.h"
#include "pci.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

/* AHCI HBA memory-mapped registers */
#define HBA_GHC_AE    (1u<<31)  /* AHCI Enable */
#define HBA_GHC_IE    (1u<<1)   /* Interrupt Enable */
#define HBA_GHC_HR    (1u<<0)   /* HBA Reset */

typedef struct {
    uint32_t cap, ghc, is, pi, vs, ccc_ctl, ccc_pts, em_loc, em_ctl, cap2, bohc;
    uint8_t  reserved[0xA0-0x2C];
    uint8_t  vendor[0x100-0xA0];
    /* Port registers follow, 0x100 + port*0x80 */
} __attribute__((packed)) hba_mem_t;

typedef struct {
    uint32_t clb, clbu, fb, fbu, is, ie, cmd, reserved0, tfd, sig;
    uint32_t ssts, sctl, serr, sact, ci, sntf, fbs;
    uint32_t reserved1[11];
    uint32_t vendor[4];
} __attribute__((packed)) hba_port_t;

/* Command List Structure (one entry = 32 bytes) */
typedef struct {
    uint16_t opts;    /* CFL[4:0], A, W, P, R, B, C, PMP[15:12] */
    uint16_t prdtl;  /* PRDT length */
    uint32_t prdbc;  /* byte count */
    uint32_t ctba, ctbau;
    uint32_t reserved[4];
} __attribute__((packed)) hba_cmd_hdr_t;

/* Physical Region Descriptor Table Entry */
typedef struct {
    uint32_t dba, dbau;
    uint32_t reserved;
    uint32_t dbc;   /* byte count - 1 | interrupt on completion bit 31 */
} __attribute__((packed)) hba_prdt_entry_t;

/* Command Table */
typedef struct {
    uint8_t           cfis[64];   /* Command FIS */
    uint8_t           acmd[16];   /* ATAPI Command */
    uint8_t           reserved[48];
    hba_prdt_entry_t  prdt[1];
} __attribute__((packed)) hba_cmd_tbl_t;

/* FIS types */
#define FIS_TYPE_REG_H2D 0x27

typedef struct {
    uint8_t  fis_type;  /* 0x27 */
    uint8_t  pmport_c;  /* bit 7 = C (command register update) */
    uint8_t  command;
    uint8_t  featurel;
    uint8_t  lba0, lba1, lba2;
    uint8_t  device;
    uint8_t  lba3, lba4, lba5;
    uint8_t  featureh;
    uint16_t count;
    uint8_t  icc, control;
    uint8_t  aux[4];
} __attribute__((packed)) fis_reg_h2d_t;

#define ATA_CMD_READ_DMA_EX  0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY_DEV 0xEC

/* Per-port state */
typedef struct {
    volatile hba_port_t *port;
    hba_cmd_hdr_t *clb;
    hba_cmd_tbl_t *ctbl;
    uint8_t       *fb;        /* FIS receive buffer */
    uint64_t       sectors;
    char           model[41];
    int            present;
} ahci_port_t;

static volatile hba_mem_t *hba  = NULL;
static ahci_port_t         aports[AHCI_MAX_PORTS];

static inline void memset0(void *d,int v,int n){uint8_t*p=d;while(n--)*p++=(uint8_t)v;}
static inline void memcpy0(void *d,const void*s,int n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}

static int port_has_device(volatile hba_port_t *port) {
    uint32_t ssts = port->ssts;
    uint8_t ipm = (uint8_t)((ssts >> 8) & 0x0F);
    uint8_t det = (uint8_t)(ssts & 0x0F);
    return det == 3 && ipm == 1;
}

static int port_start_cmd(volatile hba_port_t *port) {
    /* Wait until not busy */
    for (int i=0;i<1000000;i++) if (!(port->cmd & 0x8000)) break;
    port->cmd |= 0x10;   /* FRE */
    port->cmd |= 0x01;   /* ST */
    return 0;
}

static void port_stop_cmd(volatile hba_port_t *port) {
    port->cmd &= ~0x01;
    port->cmd &= ~0x10;
    for (int i=0;i<1000000;i++) if (!(port->cmd & 0xC000)) break;
}

static int port_issue(ahci_port_t *ap, int slot, int write,
                      uint64_t lba, uint16_t count, void *buf) {
    volatile hba_port_t *p = ap->port;

    /* Clear pending interrupts */
    p->is = (uint32_t)-1;

    hba_cmd_hdr_t *hdr = &ap->clb[slot];
    memset0(hdr, 0, sizeof(*hdr));
    hdr->opts  = (uint16_t)(sizeof(fis_reg_h2d_t)/4);  /* CFL */
    if (write) hdr->opts |= (1<<6);  /* Write */
    hdr->prdtl = 1;

    hba_cmd_tbl_t *ct = ap->ctbl;
    memset0(ct, 0, sizeof(*ct) - sizeof(hba_prdt_entry_t) + sizeof(hba_prdt_entry_t));
    hdr->ctba  = (uint32_t)(uintptr_t)ct;
    hdr->ctbau = 0;

    ct->prdt[0].dba    = (uint32_t)(uintptr_t)buf;
    ct->prdt[0].dbau   = 0;
    ct->prdt[0].dbc    = (uint32_t)(count * 512 - 1) | (1u<<31);

    /* Build H2D FIS */
    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)ct->cfis;
    fis->fis_type  = FIS_TYPE_REG_H2D;
    fis->pmport_c  = 0x80;  /* C = 1 */
    fis->command   = write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX;
    fis->device    = 0x40;  /* LBA mode */
    fis->lba0      = (uint8_t)(lba);
    fis->lba1      = (uint8_t)(lba >> 8);
    fis->lba2      = (uint8_t)(lba >> 16);
    fis->lba3      = (uint8_t)(lba >> 24);
    fis->lba4      = (uint8_t)(lba >> 32);
    fis->lba5      = (uint8_t)(lba >> 40);
    fis->count     = count;

    /* Issue command */
    p->ci = 1u << slot;

    /* Wait for completion */
    for (int i=0;i<10000000;i++) {
        if (!(p->ci & (1u<<slot))) break;
        if (p->is & (1u<<30)) return -1;  /* task file error */
    }
    return (p->tfd & 0x01) ? -1 : 0;
}

static int port_identify(ahci_port_t *ap) {
    volatile hba_port_t *p = ap->port;
    p->is = (uint32_t)-1;

    hba_cmd_hdr_t *hdr = &ap->clb[0];
    memset0(hdr, 0, sizeof(*hdr));
    hdr->opts  = sizeof(fis_reg_h2d_t)/4;
    hdr->prdtl = 1;
    hdr->ctba  = (uint32_t)(uintptr_t)ap->ctbl;

    uint16_t id_buf[256];
    memset0(id_buf, 0, sizeof(id_buf));
    ap->ctbl->prdt[0].dba  = (uint32_t)(uintptr_t)id_buf;
    ap->ctbl->prdt[0].dbc  = 511;  /* 512-1 */

    fis_reg_h2d_t *fis = (fis_reg_h2d_t *)ap->ctbl->cfis;
    memset0(fis, 0, sizeof(*fis));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->pmport_c = 0x80;
    fis->command  = ATA_CMD_IDENTIFY_DEV;
    fis->device   = 0;
    fis->count    = 1;

    p->ci = 1;
    for (int i=0;i<10000000;i++) if (!(p->ci & 1)) break;
    if (p->tfd & 0x01) return -1;

    ap->sectors = (uint64_t)id_buf[100] | ((uint64_t)id_buf[101]<<16) |
                  ((uint64_t)id_buf[102]<<32) | ((uint64_t)id_buf[103]<<48);
    if (!ap->sectors)
        ap->sectors = (uint32_t)id_buf[60] | ((uint32_t)id_buf[61]<<16);

    for (int i=0;i<20;i++) {
        ap->model[i*2]   = (char)(id_buf[27+i] >> 8);
        ap->model[i*2+1] = (char)(id_buf[27+i] & 0xFF);
    }
    ap->model[40] = 0;
    for (int i=39;i>=0&&ap->model[i]==' ';i--) ap->model[i]=0;
    return 0;
}

void ahci_init(void) {
    for (int i=0;i<AHCI_MAX_PORTS;i++) aports[i].present=0;

    pci_device_t *dev = pci_find_class(0x01, 0x06);  /* Mass storage / SATA AHCI */
    if (!dev) { terminal_printf("[ahci] no AHCI controller found\n"); return; }

    pci_enable_busmaster(dev->bus, dev->dev, dev->fn);
    uint32_t abar = dev->bar[5] & ~0xF;
    if (!abar) { terminal_printf("[ahci] no ABAR\n"); return; }

    hba = (volatile hba_mem_t *)(uintptr_t)abar;
    hba->ghc |= HBA_GHC_AE;

    uint32_t pi = hba->pi;
    for (int i=0;i<AHCI_MAX_PORTS;i++) {
        if (!(pi & (1u<<i))) continue;
        volatile hba_port_t *port = (volatile hba_port_t *)
            ((uint8_t *)hba + 0x100 + i*0x80);
        if (!port_has_device(port)) continue;

        port_stop_cmd(port);

        ahci_port_t *ap = &aports[i];
        ap->port = port;
        ap->clb  = (hba_cmd_hdr_t *)kzalloc(32 * 32);
        ap->ctbl = (hba_cmd_tbl_t *)kzalloc(sizeof(hba_cmd_tbl_t));
        ap->fb   = (uint8_t *)kzalloc(256);
        if (!ap->clb || !ap->ctbl || !ap->fb) continue;

        port->clb  = (uint32_t)(uintptr_t)ap->clb;
        port->clbu = 0;
        port->fb   = (uint32_t)(uintptr_t)ap->fb;
        port->fbu  = 0;
        port->is   = (uint32_t)-1;
        port->ie   = 0;

        port_start_cmd(port);

        if (port_identify(ap) == 0) {
            ap->present = 1;
            terminal_printf("[ahci] port %d: %s (%llu MB)\n",
                i, ap->model, ap->sectors / 2048);
        }
    }
}

int ahci_read_sectors(int port, uint64_t lba, uint16_t count, uint8_t *buf) {
    if (port<0||port>=AHCI_MAX_PORTS||!aports[port].present) return -1;
    return port_issue(&aports[port], 0, 0, lba, count, buf);
}

int ahci_write_sectors(int port, uint64_t lba, uint16_t count, const uint8_t *buf) {
    if (port<0||port>=AHCI_MAX_PORTS||!aports[port].present) return -1;
    /* AHCI write needs mutable buffer for DMA — copy to internal buf */
    uint8_t *tmp = kmalloc((size_t)count*512);
    if (!tmp) return -1;
    memcpy0(tmp, buf, count*512);
    int r = port_issue(&aports[port], 0, 1, lba, count, tmp);
    kfree(tmp);
    return r;
}

uint64_t ahci_drive_sectors(int port) {
    if (port<0||port>=AHCI_MAX_PORTS||!aports[port].present) return 0;
    return aports[port].sectors;
}

const char *ahci_drive_model(int port) {
    if (port<0||port>=AHCI_MAX_PORTS||!aports[port].present) return "";
    return aports[port].model;
}
