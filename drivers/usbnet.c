







static int     usbnet_ready = 0;
static uint8_t usbnet_mac[6] = {0x52,0x54,0x00,0x12,0x34,0x56};
static net_receive_cb_t usbnet_rx_cb = NULL;

int usbnet_probe(void) {

    pci_device_t *dev = pci_find_class(0x02, 0x00);
    if (!dev) return -1;

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




