/* scorpion OS - drivers/ata.c
   ATA/IDE PIO 28-bit LBA driver (primary + secondary channels)  */

#include "ata.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

static inline uint8_t  inb(uint16_t p){uint8_t  v;__asm__("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t inw(uint16_t p){uint16_t v;__asm__("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outb(uint16_t p,uint8_t  v){__asm__("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void outw(uint16_t p,uint16_t v){__asm__("outw %0,%1"::"a"(v),"Nd"(p));}

#define ATA_PRIMARY   0x1F0
#define ATA_SECONDARY 0x170
#define ATA_DATA      0x00
#define ATA_ERR       0x01
#define ATA_SECCOUNT  0x02
#define ATA_LBA0      0x03
#define ATA_LBA1      0x04
#define ATA_LBA2      0x05
#define ATA_DEVSEL    0x06
#define ATA_STATUS    0x07
#define ATA_CMD       0x07
#define ATA_ALT       0x206
#define ATA_SR_BSY    0x80
#define ATA_SR_DRQ    0x08
#define ATA_SR_ERR    0x01
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_FLUSH 0xE7
#define ATA_CMD_IDENT 0xEC

static uint32_t drive_sectors[4];

static uint16_t base_of(int drive) { return drive < 2 ? ATA_PRIMARY : ATA_SECONDARY; }
static int      sel_of (int drive) { return drive & 1; }

static void delay400(uint16_t base){inb(base+ATA_ALT);inb(base+ATA_ALT);inb(base+ATA_ALT);inb(base+ATA_ALT);}

static int poll(uint16_t base){
    for(int i=0;i<100000;i++){
        uint8_t s=inb(base+ATA_STATUS);
        if(!(s&ATA_SR_BSY)){
            if(s&ATA_SR_ERR) return -1;
            if(s&ATA_SR_DRQ) return 0;
        }
    }
    return -1;
}

void ata_init(void){
    for(int d=0;d<4;d++){
        drive_sectors[d]=0;
        uint16_t base=base_of(d);
        int sel=sel_of(d);
        outb(base+ATA_DEVSEL, 0xA0|(sel<<4));
        delay400(base);
        outb(base+ATA_CMD, ATA_CMD_IDENT);
        delay400(base);
        if(inb(base+ATA_STATUS)==0) continue;
        if(poll(base)!=0) continue;
        uint16_t id[256];
        for(int i=0;i<256;i++) id[i]=inw(base+ATA_DATA);
        drive_sectors[d]=((uint32_t)id[61]<<16)|id[60];
        terminal_printf("[ata]  drive %d: %u sectors (%u MB)\n",
            d, drive_sectors[d], drive_sectors[d]/2048);
    }
}

int ata_read_sectors(int drive, uint32_t lba, uint8_t count, void *buf){
    uint16_t base=base_of(drive); int sel=sel_of(drive);
    uint16_t *ptr=(uint16_t*)buf;
    outb(base+ATA_DEVSEL, 0xE0|(sel<<4)|((lba>>24)&0xF));
    outb(base+0x01, 0);
    outb(base+ATA_SECCOUNT, count);
    outb(base+ATA_LBA0,  lba     &0xFF);
    outb(base+ATA_LBA1, (lba>>8) &0xFF);
    outb(base+ATA_LBA2, (lba>>16)&0xFF);
    outb(base+ATA_CMD, ATA_CMD_READ);
    for(int s=0;s<count;s++){
        if(poll(base)!=0) return -1;
        for(int i=0;i<256;i++) *ptr++=inw(base+ATA_DATA);
    }
    return 0;
}

int ata_write_sectors(int drive, uint32_t lba, uint8_t count, const void *buf){
    uint16_t base=base_of(drive); int sel=sel_of(drive);
    const uint16_t *ptr=(const uint16_t*)buf;
    outb(base+ATA_DEVSEL, 0xE0|(sel<<4)|((lba>>24)&0xF));
    outb(base+0x01, 0);
    outb(base+ATA_SECCOUNT, count);
    outb(base+ATA_LBA0,  lba     &0xFF);
    outb(base+ATA_LBA1, (lba>>8) &0xFF);
    outb(base+ATA_LBA2, (lba>>16)&0xFF);
    outb(base+ATA_CMD, ATA_CMD_WRITE);
    for(int s=0;s<count;s++){
        delay400(base);
        for(int i=0;i<256;i++) outw(base+ATA_DATA,*ptr++);
    }
    outb(base+ATA_CMD, ATA_CMD_FLUSH);
    poll(base);
    return 0;
}

uint32_t ata_drive_sectors(int drive){ return drive_sectors[drive]; }
