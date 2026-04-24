/* scorpion OS - drivers/usbnet.c
   USB network adapter driver (CDC-ECM / RNDIS over USB).
   Without a full HCD stack we detect ASIX/CDC adapters via PCI/USB
   enumeration and fall back gracefully. Real TX/RX would require
   full EHCI/xHCI support; this provides the interface so the kernel
   can probe all adapters in order and skip gracefully.              */

#include "usbnet.h"
#include "pci.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

static int     usbnet_ready = 0;
static uint8_t usbnet_mac[6] = {0x52,0x54,0x00,0x12,0x34,0x56};
static net_receive_cb_t usbnet_rx_cb = NULL;

int usbnet_probe(void) {
    /* Look for known USB NIC PCI IDs:
       ASIX AX88179: class 02 (network), sometimes shows up as USB device.
       Without a real HCD we can't enumerate USB — return -1 to let the
       kernel fall through to the next adapter.                          */
    pci_device_t *dev = pci_find_class(0x02, 0x00);
    if (!dev) return -1;
    /* Already handled by rndis/e1000/virtio; usbnet is last resort */
    (void)dev;
    terminal_printf("[usbnet] no USB NIC (HCD not implemented)\n");
    return -1;
}

int usbnet_send(const uint8_t *data, size_t len) {
    (void)data; (void)len;
    return -1;
}
void usbnet_poll(void) { (void)usbnet_ready; }
void usbnet_get_mac(uint8_t mac[6]) { for(int i=0;i<6;i++) mac[i]=usbnet_mac[i]; }
void usbnet_set_rx_cb(net_receive_cb_t cb) { usbnet_rx_cb=cb; (void)usbnet_rx_cb; }
