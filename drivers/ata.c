/* scorpion OS - drivers/ata.c
   ATA PIO driver for up to 4 drives (primary + secondary bus, master + slave).
   Uses 28-bit LBA, polling (no DMA).  Supports drives up to 128 GiB.        */

#include "ata.h"
#include "../kernel/terminal.h"
#include <stdint.h>

/* ATA registers (primary bus = 0x1F0, secondary = 0x170) */
#define ATA_REG_DATA(base)     ((base)+0)
#define ATA_REG_ERROR(base)    ((base)+1)
#define ATA_REG_SECCOUNT(base) ((base)+2)
#define ATA_REG_LBA0(base)     ((base)+3)
#define ATA_REG_LBA1(base)     ((base)+4)
#define ATA_REG_LBA2(base)     ((base)+5)
#define ATA_REG_DRIVE(base)    ((base)+6)
#define ATA_REG_STATUS(base)   ((base)+7)
#define ATA_REG_CMD(base)      ((base)+7)
#define ATA_REG_ALTSTATUS(base) ((base)+0x206)  /* 0x3F6 / 0x376 */

#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01

#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC

static inline uint8_t  inb_a(uint16_t p){uint8_t  v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw_a(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb_a(uint16_t p,uint8_t  v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw_a(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}

typedef struct {
    uint16_t base;     /* I/O base: 0x1F0 or 0x170 */
    int      slave;    /* 0=master, 1=slave */
    uint32_t sectors;  /* total LBA28 sector count (0 = not present) */
    char     model[41];
} ata_drive_t;

static ata_drive_t drives[4];  /* 0,1=primary; 2,3=secondary */

static int ata_poll(uint16_t base) {
    for (int i = 0; i < 4; i++) inb_a(ATA_REG_ALTSTATUS(base));  /* 400ns delay */
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb_a(ATA_REG_STATUS(base));
        if (s & ATA_STATUS_BSY) continue;
        if (s & ATA_STATUS_ERR) return -1;
        if (s & ATA_STATUS_DRQ) return 0;
    }
    return -1;  /* timeout */
}

static int ata_identify(uint16_t base, int slave, ata_drive_t *d) {
    outb_a(ATA_REG_DRIVE(base), (uint8_t)(slave ? 0xB0 : 0xA0));
    outb_a(ATA_REG_SECCOUNT(base), 0);
    outb_a(ATA_REG_LBA0(base), 0);
    outb_a(ATA_REG_LBA1(base), 0);
    outb_a(ATA_REG_LBA2(base), 0);
    outb_a(ATA_REG_CMD(base), ATA_CMD_IDENTIFY);

    uint8_t s = inb_a(ATA_REG_STATUS(base));
    if (!s) return -1;  /* no drive */

    if (ata_poll(base) < 0) return -1;

    /* Read 256 words of IDENTIFY data */
    uint16_t id[256];
    for (int i = 0; i < 256; i++) id[i] = inw_a(ATA_REG_DATA(base));

    /* Word 60-61: LBA28 sector count */
    d->sectors = (uint32_t)id[60] | ((uint32_t)id[61] << 16);
    if (!d->sectors) return -1;

    /* Model string: words 27-46, big-endian byte pairs */
    for (int i = 0; i < 20; i++) {
        d->model[i*2]   = (char)(id[27+i] >> 8);
        d->model[i*2+1] = (char)(id[27+i] & 0xFF);
    }
    d->model[40] = 0;
    /* Trim trailing spaces */
    for (int i = 39; i >= 0 && d->model[i] == ' '; i--) d->model[i] = 0;

    d->base  = base;
    d->slave = slave;
    return 0;
}

void ata_init(void) {
    static const struct { uint16_t base; int slave; } cfg[4] = {
        {0x1F0,0},{0x1F0,1},{0x170,0},{0x170,1}
    };
    for (int i = 0; i < 4; i++) {
        drives[i].sectors = 0;
        if (ata_identify(cfg[i].base, cfg[i].slave, &drives[i]) == 0) {
            terminal_printf("[ata] drive %d: %s (%u MB)\n",
                i, drives[i].model, drives[i].sectors / 2048);
        }
    }
}

static int drive_valid(int drive) {
    return drive >= 0 && drive < 4 && drives[drive].sectors > 0;
}

int ata_read_sectors(int drive, uint32_t lba, uint8_t count, uint8_t *buf) {
    if (!drive_valid(drive) || !count) return -1;
    uint16_t base  = drives[drive].base;
    int      slave = drives[drive].slave;

    outb_a(ATA_REG_DRIVE(base),  (uint8_t)(0xE0 | (slave << 4) | ((lba >> 24) & 0x0F)));
    outb_a(ATA_REG_SECCOUNT(base), count);
    outb_a(ATA_REG_LBA0(base), (uint8_t)(lba));
    outb_a(ATA_REG_LBA1(base), (uint8_t)(lba >> 8));
    outb_a(ATA_REG_LBA2(base), (uint8_t)(lba >> 16));
    outb_a(ATA_REG_CMD(base),  ATA_CMD_READ);

    for (int s = 0; s < count; s++) {
        if (ata_poll(base) < 0) return -1;
        for (int i = 0; i < 256; i++) {
            uint16_t w = inw_a(ATA_REG_DATA(base));
            buf[s*512 + i*2]     = (uint8_t)(w & 0xFF);
            buf[s*512 + i*2 + 1] = (uint8_t)(w >> 8);
        }
    }
    return 0;
}

int ata_write_sectors(int drive, uint32_t lba, uint8_t count, const uint8_t *buf) {
    if (!drive_valid(drive) || !count) return -1;
    uint16_t base  = drives[drive].base;
    int      slave = drives[drive].slave;

    outb_a(ATA_REG_DRIVE(base),  (uint8_t)(0xE0 | (slave << 4) | ((lba >> 24) & 0x0F)));
    outb_a(ATA_REG_SECCOUNT(base), count);
    outb_a(ATA_REG_LBA0(base), (uint8_t)(lba));
    outb_a(ATA_REG_LBA1(base), (uint8_t)(lba >> 8));
    outb_a(ATA_REG_LBA2(base), (uint8_t)(lba >> 16));
    outb_a(ATA_REG_CMD(base),  ATA_CMD_WRITE);

    for (int s = 0; s < count; s++) {
        if (ata_poll(base) < 0) return -1;
        for (int i = 0; i < 256; i++) {
            uint16_t w = (uint16_t)buf[s*512 + i*2] | ((uint16_t)buf[s*512 + i*2+1] << 8);
            outw_a(ATA_REG_DATA(base), w);
        }
        /* Flush write cache */
        outb_a(ATA_REG_CMD(base), 0xE7);
        for (int i=0;i<4;i++) inb_a(ATA_REG_ALTSTATUS(base));
        for (int i=0;i<1000000;i++) if (!(inb_a(ATA_REG_STATUS(base)) & ATA_STATUS_BSY)) break;
    }
    return 0;
}

uint32_t ata_drive_sectors(int drive) {
    if (!drive_valid(drive)) return 0;
    return drives[drive].sectors;
}

int ata_drive_present(int drive) {
    return drive_valid(drive);
}
