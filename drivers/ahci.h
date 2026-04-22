







void     ahci_init(void);
int      ahci_read_sectors(int port, uint64_t lba, uint16_t count, uint8_t *buf);
int      ahci_write_sectors(int port, uint64_t lba, uint16_t count, const uint8_t *buf);
uint64_t ahci_drive_sectors(int port);
const char *ahci_drive_model(int port);






