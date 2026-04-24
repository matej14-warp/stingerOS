/* scorpion OS - drivers/ata.h */
#ifndef ATA_H
#define ATA_H
#include <stdint.h>

void     ata_init(void);
int      ata_read_sectors(int drive, uint32_t lba, uint8_t count, uint8_t *buf);
int      ata_write_sectors(int drive, uint32_t lba, uint8_t count, const uint8_t *buf);
uint32_t ata_drive_sectors(int drive);  /* 0 if not present */
int      ata_drive_present(int drive);

#endif /* ATA_H */
