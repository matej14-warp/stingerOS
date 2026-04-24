#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stddef.h>

/* Endian helpers (kernel runs on LE x86) */
static inline uint16_t htons(uint16_t v) { return (v>>8)|(v<<8); }
static inline uint16_t ntohs(uint16_t v) { return htons(v); }
static inline uint32_t htonl(uint32_t v) {
    return ((v>>24)&0xFF)|((v>>8)&0xFF00)|((v<<8)&0xFF0000)|((v<<24)&0xFF000000);
}
static inline uint32_t ntohl(uint32_t v) { return htonl(v); }

/* Ethernet */
#define ETH_TYPE_IP  0x0800
#define ETH_TYPE_ARP 0x0806

typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

/* ARP */
#define ARP_REQUEST 1
#define ARP_REPLY   2

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

/* IP */
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

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

/* ICMP */
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed)) icmp_hdr_t;

/* TCP */
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

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

/* UDP */
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;

/* DHCP (simplified) */
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_DISCOVER    1
#define DHCP_OFFER       2
#define DHCP_REQUEST     3
#define DHCP_ACK         5

/* Network config */
typedef struct {
    uint8_t  ip[4];
    uint8_t  mask[4];
    uint8_t  gw[4];
    uint8_t  dns[4];
    uint8_t  mac[6];
    int      configured;
    /* DNS resolution scratch space - filled by net_receive_frame on DNS reply */
    uint8_t  dns_result[4];
    int      dns_resolved;
} net_config_t;

/* TCP socket (simple blocking) */
#define TCP_STATE_CLOSED      0
#define TCP_STATE_SYN_SENT    1
#define TCP_STATE_ESTABLISHED 2
#define TCP_STATE_FIN_WAIT    3

typedef struct {
    uint8_t  state;
    uint8_t  remote_ip[4];
    uint8_t  remote_mac[6];
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t seq;
    uint32_t ack;
    /* rx ring buffer */
    uint8_t  rx_ring[8192];
    uint32_t rx_head, rx_tail;
} tcp_socket_t;

/* Public API */
void net_init(void);
int  net_dhcp(void);
void net_get_config(net_config_t *cfg);
int  net_arp_resolve(const uint8_t *ip, uint8_t *mac_out);

tcp_socket_t *tcp_connect(const uint8_t *ip, uint16_t port);
int           tcp_send(tcp_socket_t *s, const uint8_t *data, size_t len);
int           tcp_recv(tcp_socket_t *s, uint8_t *buf, size_t maxlen, uint32_t timeout_ms);
void          tcp_close(tcp_socket_t *s);

/* HTTP GET - returns HTTP body in allocated buffer, caller must kfree */
int http_get(const char *host, const char *path, uint8_t **body_out, size_t *len_out);

/* Called by RNDIS receive callback */
void net_receive_frame(const uint8_t *frame, size_t len);

uint16_t net_checksum(const void *data, size_t len);

#endif /* NET_H */
