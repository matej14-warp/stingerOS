#ifndef ATA_H
#define ATA_H
#include <stdint.h>

void     ata_init(void);
int      ata_read_sectors(int drive, uint32_t lba, uint8_t count, void *buf);
int      ata_write_sectors(int drive, uint32_t lba, uint8_t count, const void *buf);
uint32_t ata_drive_sectors(int drive);

#endif
