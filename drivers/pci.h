/* scorpion OS - drivers/pci.h */
#ifndef PCI_H
#define PCI_H
#include <stdint.h>

#define PCI_VENDOR_NONE 0xFFFF

typedef struct {
    uint8_t  bus, dev, fn;
    uint16_t vendor, device;
    uint8_t  class_code, subclass, prog_if;
    uint8_t  header_type;
    uint32_t bar[6];
    uint8_t  irq;
} pci_device_t;

#define PCI_MAX_DEVICES 64
extern pci_device_t g_pci_devices[];
extern int          g_pci_count;

void     pci_scan(void);
uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg);
void     pci_write(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t reg, uint32_t val);
void     pci_enable_busmaster(uint8_t bus, uint8_t dev, uint8_t fn);
pci_device_t *pci_find(uint16_t vendor, uint16_t device);
pci_device_t *pci_find_class(uint8_t class_code, uint8_t subclass);
void     pci_list(void);

#endif /* PCI_H */
