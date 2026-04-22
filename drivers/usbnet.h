





int  usbnet_probe(void);
int  usbnet_send(const uint8_t *data, size_t len);
void usbnet_poll(void);
void usbnet_get_mac(uint8_t mac[6]);
void usbnet_set_rx_cb(net_receive_cb_t cb);





