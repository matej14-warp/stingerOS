/* scorpion OS - drivers/rndis.h
   RTL8139 / RNDIS-style Ethernet driver.
   Supports: RTL8139 PCI Ethernet (PCI 0x10EC:0x8139) via I/O BAR0.  */
#ifndef RNDIS_H
#define RNDIS_H
#include <stdint.h>
#include <stddef.h>
typedef void (*net_receive_cb_t)(const uint8_t *data, size_t len);
int  rndis_init(void);
int  rndis_send(const uint8_t *data, size_t len);
void rndis_poll(void);
void rndis_get_mac(uint8_t mac[6]);
void rndis_set_receive_cb(net_receive_cb_t cb);
#endif /* RNDIS_H */
