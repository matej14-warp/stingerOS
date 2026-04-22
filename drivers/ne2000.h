






int  ne2000_init(void);
int  ne2000_send(const uint8_t *data, size_t len);
void ne2000_poll(void);
void ne2000_get_mac(uint8_t mac[6]);
void ne2000_set_rx_cb(net_receive_cb_t cb);
int  ne2000_is_ready(void);






