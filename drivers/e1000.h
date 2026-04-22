





int  e1000_init(void);
int  e1000_send(const uint8_t *data, size_t len);
void e1000_poll(void);
void e1000_get_mac(uint8_t mac[6]);
void e1000_set_rx_cb(net_receive_cb_t cb);
int  e1000_is_ready(void);





