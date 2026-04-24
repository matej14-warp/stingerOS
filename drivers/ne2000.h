/* scorpion OS - drivers/ne2000.h
   NE2000 (ISA/PCI) Ethernet driver interface.
   PCI: 0x10EC:0x8029 (QEMU: -device ne2k_pci)
   ISA fallback: probes 0x300, 0x280, 0x320, 0x340  */
#ifndef NE2000_H
#define NE2000_H
#include <stdint.h>
#include <stddef.h>
#include "rndis.h"

int  ne2000_init(void);
int  ne2000_send(const uint8_t *data, size_t len);
void ne2000_poll(void);
void ne2000_get_mac(uint8_t mac[6]);
void ne2000_set_rx_cb(net_receive_cb_t cb);
int  ne2000_is_ready(void);

#endif /* NE2000_H */
