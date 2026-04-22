






static inline uint16_t htons(uint16_t v) { return (v>>8)|(v<<8); }
static inline uint16_t ntohs(uint16_t v) { return htons(v); }
static inline uint32_t htonl(uint32_t v) {
    return ((v>>24)&0xFF)|((v>>8)&0xFF00)|((v<<8)&0xFF0000)|((v<<24)&0xFF000000);
}
static inline uint32_t ntohl(uint32_t v) { return htonl(v); }





typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;





typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[6];
    uint8_t  spa[4];
    uint8_t  tha[6];
    uint8_t  tpa[4];
} __attribute__((packed)) arp_pkt_t;






typedef struct {
    uint8_t  ver_ihl;
    uint8_t  dscp;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint8_t  src[4];
    uint8_t  dst[4];
} __attribute__((packed)) ip_hdr_t;


typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed)) icmp_hdr_t;


typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  data_offset;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_hdr_t;








typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;










typedef struct {
    uint8_t  ip[4];
    uint8_t  mask[4];
    uint8_t  gw[4];
    uint8_t  dns[4];
    uint8_t  mac[6];
    int      configured;

    uint8_t  dns_result[4];
    int      dns_resolved;
} net_config_t;







typedef struct {
    uint8_t  state;
    uint8_t  remote_ip[4];
    uint8_t  remote_mac[6];
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t seq;
    uint32_t ack;

    uint8_t  rx_ring[8192];
    uint32_t rx_head, rx_tail;
} tcp_socket_t;


void net_init(void);
void net_set_mac(const uint8_t mac[6]);
int  net_dhcp(void);
int  net_dhcp_once(void);
void net_get_config(net_config_t *cfg);
void net_set_poll_fn(void (*fn)(void));
void net_set_send_fn(int  (*fn)(const uint8_t*, size_t));
void net_set_config(const net_config_t *cfg);
int  net_arp_resolve(const uint8_t *ip, uint8_t *mac_out);
const uint8_t *net_get_route(const uint8_t *dest_ip);

tcp_socket_t *tcp_connect(const uint8_t *ip, uint16_t port);
int           tcp_send(tcp_socket_t *s, const uint8_t *data, size_t len);
int           tcp_recv(tcp_socket_t *s, uint8_t *buf, size_t maxlen, uint32_t timeout_ms);
void          tcp_close(tcp_socket_t *s);


int http_get(const char *host, const char *path, uint8_t **body_out, size_t *len_out);


void net_receive_frame(const uint8_t *frame, size_t len);

uint16_t net_checksum(const void *data, size_t len);


extern volatile int g_net_cancel;


int net_send_udp_raw(const uint8_t *dst_ip, uint16_t src_port,
                     uint16_t dst_port, const uint8_t *data, size_t len);
int net_recv_udp_raw(uint16_t src_port, uint8_t *buf, size_t maxlen,
                     uint32_t timeout_ms);
void udp_rx_deliver(uint16_t dst_port, const uint8_t *data, size_t len);





