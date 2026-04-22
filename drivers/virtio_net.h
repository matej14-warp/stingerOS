




typedef void (*net_receive_cb_t)(const uint8_t *data, size_t len);
int  virtio_net_init(void);
int  virtio_net_send(const uint8_t *data, size_t len);
void virtio_net_poll(void);
void virtio_net_get_mac(uint8_t mac[6]);
void virtio_net_set_rx_cb(net_receive_cb_t cb);
int  virtio_net_is_ready(void);





