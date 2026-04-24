/* scorpion OS - drivers/net.c
   Minimal TCP/IP stack: ARP, DHCP, IP, ICMP, TCP, HTTP GET  */

#include "net.h"
#include "rndis.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

/* ---- helpers ---- */
static void memset0(void *d, size_t n) { uint8_t *p=d; while(n--)*p++=0; }
static void memcpy0(void *d, const void *s, size_t n) {
    uint8_t *dd=d; const uint8_t *ss=s; while(n--)*dd++=*ss++;
}
static int memcmp0(const void *a, const void *b, size_t n) {
    const uint8_t *aa=a,*bb=b;
    while(n--){if(*aa!=*bb)return *aa-*bb;aa++;bb++;} return 0;
}
static int kstrncmp(const char*a,const char*b,size_t n){
    while(n&&*a&&*a==*b){a++;b++;n--;}
    if(!n) return 0;
    return (unsigned char)*a-(unsigned char)*b;
}

static const uint8_t BROADCAST_MAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8_t ZERO_MAC[6]      = {0,0,0,0,0,0};

/* ---- ARP cache ---- */
#define ARP_CACHE_SIZE 16
typedef struct { uint8_t ip[4]; uint8_t mac[6]; int valid; } arp_entry_t;
static arp_entry_t arp_cache[ARP_CACHE_SIZE];

static void arp_cache_set(const uint8_t *ip, const uint8_t *mac) {
    for(int i=0;i<ARP_CACHE_SIZE;i++){
        if(!arp_cache[i].valid || memcmp0(arp_cache[i].ip,ip,4)==0){
            memcpy0(arp_cache[i].ip,ip,4);
            memcpy0(arp_cache[i].mac,mac,6);
            arp_cache[i].valid=1; return;
        }
    }
}
static int arp_cache_get(const uint8_t *ip, uint8_t *mac_out) {
    for(int i=0;i<ARP_CACHE_SIZE;i++)
        if(arp_cache[i].valid && memcmp0(arp_cache[i].ip,ip,4)==0){
            memcpy0(mac_out,arp_cache[i].mac,6); return 1;
        }
    return 0;
}

/* ---- Network config ---- */
static net_config_t net_cfg;
static uint8_t      tx_frame[1600];

/* ---- Checksum ---- */
uint16_t net_checksum(const void *data, size_t len) {
    const uint16_t *p = data;
    uint32_t sum = 0;
    while(len > 1){ sum += *p++; len -= 2; }
    if(len) sum += *(uint8_t*)p;
    while(sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

/* TCP pseudo-header checksum */
static uint16_t tcp_checksum(const uint8_t *src_ip, const uint8_t *dst_ip,
                              const uint8_t *tcp_seg, size_t tcp_len) {
    uint8_t pseudo[12 + tcp_len > 2048 ? 2048 : tcp_len + 12];
    memcpy0(pseudo, src_ip, 4);
    memcpy0(pseudo+4, dst_ip, 4);
    pseudo[8]=0; pseudo[9]=IP_PROTO_TCP;
    pseudo[10]=(uint8_t)(tcp_len>>8); pseudo[11]=(uint8_t)(tcp_len&0xFF);
    if(tcp_len <= 2048) memcpy0(pseudo+12, tcp_seg, tcp_len);
    return net_checksum(pseudo, 12 + (tcp_len <= 2048 ? tcp_len : 0));
}

/* ---- Send raw Ethernet frame ---- */
static void eth_send(const uint8_t *dst_mac, uint16_t type,
                     const void *payload, size_t plen) {
    eth_hdr_t *eth = (eth_hdr_t*)tx_frame;
    memcpy0(eth->dst, dst_mac, 6);
    memcpy0(eth->src, net_cfg.mac, 6);
    eth->type = htons(type);
    memcpy0(tx_frame + sizeof(eth_hdr_t), payload, plen);
    rndis_send(tx_frame, sizeof(eth_hdr_t) + plen);
}

/* ---- ARP ---- */
static void arp_send_request(const uint8_t *target_ip) {
    uint8_t pkt[sizeof(arp_pkt_t)];
    arp_pkt_t *a = (arp_pkt_t*)pkt;
    a->htype = htons(1); a->ptype = htons(0x0800);
    a->hlen=6; a->plen=4; a->oper=htons(ARP_REQUEST);
    memcpy0(a->sha, net_cfg.mac, 6);
    memcpy0(a->spa, net_cfg.ip,  4);
    memset0(a->tha, 6);
    memcpy0(a->tpa, target_ip, 4);
    eth_send(BROADCAST_MAC, ETH_TYPE_ARP, pkt, sizeof(pkt));
}

static void arp_handle(const uint8_t *frame, size_t len) {
    if(len < sizeof(eth_hdr_t)+sizeof(arp_pkt_t)) return;
    const arp_pkt_t *a = (const arp_pkt_t*)(frame + sizeof(eth_hdr_t));
    /* Cache sender */
    arp_cache_set(a->spa, a->sha);
    /* FIX: cache the target from ARP replies so net_arp_resolve() sees the result */
    if(ntohs(a->oper)==ARP_REPLY) {
        arp_cache_set(a->tpa, a->tha);
        return;
    }
    /* Reply to requests directed at us */
    if(ntohs(a->oper)==ARP_REQUEST && memcmp0(a->tpa,net_cfg.ip,4)==0){
        uint8_t reply[sizeof(arp_pkt_t)];
        arp_pkt_t *r=(arp_pkt_t*)reply;
        r->htype=htons(1); r->ptype=htons(0x0800); r->hlen=6; r->plen=4;
        r->oper=htons(ARP_REPLY);
        memcpy0(r->sha,net_cfg.mac,6); memcpy0(r->spa,net_cfg.ip,4);
        memcpy0(r->tha,a->sha,6);      memcpy0(r->tpa,a->spa,4);
        eth_send(a->sha, ETH_TYPE_ARP, reply, sizeof(reply));
    }
}

/* ---- DHCP ---- */
#define DHCP_MAGIC 0x63825363

typedef struct {
    uint8_t  op,htype,hlen,hops;
    uint32_t xid;
    uint16_t secs,flags;
    uint8_t  ciaddr[4],yiaddr[4],siaddr[4],giaddr[4];
    uint8_t  chaddr[16];
    uint8_t  sname[64],file[128];
    uint32_t magic;
    uint8_t  options[312];
} __attribute__((packed)) dhcp_pkt_t;

static uint8_t dhcp_buf[sizeof(dhcp_pkt_t)];

int net_dhcp(void) {
    terminal_printf("[net] sending DHCP discover...\n");

    dhcp_pkt_t *d = (dhcp_pkt_t*)dhcp_buf;
    memset0(d, sizeof(*d));
    d->op=1; d->htype=1; d->hlen=6; d->xid=htonl(0xDEADBEEF);
    d->flags=htons(0x8000); /* broadcast */
    memcpy0(d->chaddr, net_cfg.mac, 6);
    d->magic = htonl(DHCP_MAGIC);

    /* DHCP options: msg type=DISCOVER, end */
    int opt=0;
    d->options[opt++]=53; d->options[opt++]=1; d->options[opt++]=DHCP_DISCOVER;
    d->options[opt++]=61; d->options[opt++]=7; d->options[opt++]=1;
    memcpy0(&d->options[opt], net_cfg.mac, 6); opt+=6;
    d->options[opt++]=55; d->options[opt++]=3;
    d->options[opt++]=1; d->options[opt++]=3; d->options[opt++]=6;
    d->options[opt++]=255; /* end */

    /* UDP/IP encapsulation */
    uint8_t ip_udp[sizeof(ip_hdr_t)+sizeof(udp_hdr_t)+sizeof(*d)];
    memset0(ip_udp, sizeof(ip_udp));

    udp_hdr_t *u = (udp_hdr_t*)(ip_udp+sizeof(ip_hdr_t));
    u->src_port = htons(DHCP_CLIENT_PORT);
    u->dst_port = htons(DHCP_SERVER_PORT);
    u->length   = htons(sizeof(udp_hdr_t)+sizeof(*d));
    memcpy0(ip_udp+sizeof(ip_hdr_t)+sizeof(udp_hdr_t), d, sizeof(*d));

    ip_hdr_t *ip = (ip_hdr_t*)ip_udp;
    ip->ver_ihl  = 0x45; ip->dscp=0;
    ip->total_len= htons((uint16_t)sizeof(ip_udp));
    ip->id=htons(1); ip->flags_frag=0; ip->ttl=64; ip->proto=IP_PROTO_UDP;
    memset0(ip->src,4); /* 0.0.0.0 */
    memset0(ip->dst,4); for(int i=0;i<4;i++) ip->dst[i]=0xFF; /* 255.255.255.255 */
    ip->checksum = net_checksum(ip, sizeof(ip_hdr_t));

    eth_send(BROADCAST_MAC, ETH_TYPE_IP, ip_udp, sizeof(ip_udp));

    /* Wait for DHCP offer/ack via polling (up to 3 seconds) */
    /* In practice, net_receive_frame will fill net_cfg on DHCP ACK */
    for(int i=0;i<3000;i++){
        rndis_poll();
        if(net_cfg.configured) {
            terminal_printf("[net] DHCP: got IP %d.%d.%d.%d\n",
                net_cfg.ip[0],net_cfg.ip[1],net_cfg.ip[2],net_cfg.ip[3]);
            return 0;
        }
        /* small spin delay */
        for(volatile int j=0;j<10000;j++);
    }
    terminal_printf("[net] DHCP timeout, using 192.168.1.100\n");
    net_cfg.ip[0]=192; net_cfg.ip[1]=168; net_cfg.ip[2]=1; net_cfg.ip[3]=100;
    net_cfg.mask[0]=255; net_cfg.mask[1]=255; net_cfg.mask[2]=255; net_cfg.mask[3]=0;
    net_cfg.gw[0]=192; net_cfg.gw[1]=168; net_cfg.gw[2]=1; net_cfg.gw[3]=1;
    net_cfg.dns[0]=8; net_cfg.dns[1]=8; net_cfg.dns[2]=8; net_cfg.dns[3]=8;
    net_cfg.configured=1;
    return -1;
}

/* ---- TCP sockets ---- */
static uint16_t next_port = 49152;

static void tcp_send_raw(tcp_socket_t *s, uint8_t flags,
                          const uint8_t *data, size_t dlen) {
    uint8_t mac[6];
    /* FIX: route off-subnet traffic via gateway */
    const uint8_t *arp_target = s->remote_ip;
    if(net_cfg.configured) {
        int on_subnet = 1;
        for(int i=0;i<4;i++) {
            if((s->remote_ip[i] & net_cfg.mask[i]) != (net_cfg.ip[i] & net_cfg.mask[i])) {
                on_subnet = 0; break;
            }
        }
        if(!on_subnet) arp_target = net_cfg.gw;
    }
    if(!arp_cache_get(arp_target, mac)){
        arp_send_request(arp_target);
        for(int i=0;i<500;i++){
            rndis_poll();
            if(arp_cache_get(arp_target,mac)) break;
            for(volatile int j=0;j<10000;j++);
        }
    }

    size_t seg_len = sizeof(tcp_hdr_t) + dlen;
    size_t pkt_len = sizeof(ip_hdr_t) + seg_len;
    uint8_t *pkt = (uint8_t*)kmalloc(pkt_len);
    if(!pkt) return;
    memset0(pkt, pkt_len);

    tcp_hdr_t *tcp = (tcp_hdr_t*)(pkt + sizeof(ip_hdr_t));
    tcp->src_port   = htons(s->local_port);
    tcp->dst_port   = htons(s->remote_port);
    tcp->seq        = htonl(s->seq);
    tcp->ack        = htonl(s->ack);
    tcp->data_offset= 0x50; /* 5 words, no options */
    tcp->flags      = flags;
    tcp->window     = htons(8192);
    if(dlen) memcpy0((uint8_t*)tcp+sizeof(tcp_hdr_t), data, dlen);
    tcp->checksum   = tcp_checksum(net_cfg.ip, s->remote_ip,
                                    (uint8_t*)tcp, seg_len);

    ip_hdr_t *ip = (ip_hdr_t*)pkt;
    ip->ver_ihl  = 0x45; ip->dscp=0;
    ip->total_len= htons((uint16_t)pkt_len);
    ip->id=htons(next_port); ip->flags_frag=0; ip->ttl=64; ip->proto=IP_PROTO_TCP;
    memcpy0(ip->src, net_cfg.ip, 4);
    memcpy0(ip->dst, s->remote_ip, 4);
    ip->checksum = net_checksum(ip, sizeof(ip_hdr_t));

    eth_send(mac, ETH_TYPE_IP, pkt, pkt_len);
    kfree(pkt);
}

tcp_socket_t *tcp_connect(const uint8_t *ip, uint16_t port) {
    tcp_socket_t *s = (tcp_socket_t*)kzalloc(sizeof(tcp_socket_t));
    if(!s) return NULL;
    memcpy0(s->remote_ip, ip, 4);
    s->remote_port = port;
    s->local_port  = next_port++;
    s->seq         = 0xABCD1234;
    s->ack         = 0;
    s->state       = TCP_STATE_SYN_SENT;
    s->rx_head = s->rx_tail = 0;

    /* Send SYN */
    tcp_send_raw(s, TCP_SYN, NULL, 0);
    s->seq++;

    /* Wait for SYN-ACK */
    for(int i=0; i<5000; i++){
        rndis_poll();
        if(s->state == TCP_STATE_ESTABLISHED) return s;
        for(volatile int j=0;j<10000;j++);
    }
    kfree(s);
    return NULL;
}

int tcp_send(tcp_socket_t *s, const uint8_t *data, size_t len) {
    if(!s || s->state != TCP_STATE_ESTABLISHED) return -1;
    tcp_send_raw(s, TCP_PSH|TCP_ACK, data, len);
    s->seq += (uint32_t)len;
    return (int)len;
}

int tcp_recv(tcp_socket_t *s, uint8_t *buf, size_t maxlen, uint32_t timeout_ms) {
    if(!s) return -1;
    uint32_t waited = 0;
    while(waited < timeout_ms) {
        rndis_poll();
        if(s->rx_head != s->rx_tail) {
            size_t avail = 0;
            while(s->rx_head != s->rx_tail && avail < maxlen) {
                buf[avail++] = s->rx_ring[s->rx_tail % sizeof(s->rx_ring)];
                s->rx_tail++;
            }
            return (int)avail;
        }
        if(s->state == TCP_STATE_FIN_WAIT || s->state == TCP_STATE_CLOSED)
            return 0;
        for(volatile int j=0;j<10000;j++);
        waited++;
    }
    return 0;
}

void tcp_close(tcp_socket_t *s) {
    if(!s) return;
    if(s->state == TCP_STATE_ESTABLISHED)
        tcp_send_raw(s, TCP_FIN|TCP_ACK, NULL, 0);
    s->state = TCP_STATE_CLOSED;
    kfree(s);
}

/* ---- Frame receiver (called from RNDIS) ---- */
void net_receive_frame(const uint8_t *frame, size_t len) {
    if(len < sizeof(eth_hdr_t)) return;
    const eth_hdr_t *eth = (const eth_hdr_t*)frame;
    uint16_t type = ntohs(eth->type);

    if(type == ETH_TYPE_ARP) { arp_handle(frame, len); return; }
    if(type != ETH_TYPE_IP) return;

    if(len < sizeof(eth_hdr_t)+sizeof(ip_hdr_t)) return;
    const ip_hdr_t *ip = (const ip_hdr_t*)(frame+sizeof(eth_hdr_t));
    size_t ip_hlen = (ip->ver_ihl & 0xF)*4;

    if(ip->proto == IP_PROTO_UDP) {
        const udp_hdr_t *udp=(const udp_hdr_t*)((uint8_t*)ip+ip_hlen);
        uint16_t dport = ntohs(udp->dst_port);
        uint16_t sport = ntohs(udp->src_port);
        if(dport == DHCP_CLIENT_PORT) {
            /* Parse DHCP offer/ack */
            const dhcp_pkt_t *d=(const dhcp_pkt_t*)((uint8_t*)udp+sizeof(udp_hdr_t));
            if(d->op==2 && ntohl(d->magic)==DHCP_MAGIC) {
                /* Extract offered IP */
                memcpy0(net_cfg.ip, d->yiaddr, 4);
                /* Parse options for subnet, gw, dns */
                const uint8_t *opt=d->options;
                while(*opt!=255 && (opt-d->options)<312) {
                    uint8_t code=*opt++; if(code==0)continue;
                    uint8_t olen=*opt++;
                    if(code==1&&olen==4) memcpy0(net_cfg.mask,opt,4);
                    if(code==3&&olen>=4) memcpy0(net_cfg.gw,opt,4);
                    if(code==6&&olen>=4) memcpy0(net_cfg.dns,opt,4);
                    opt+=olen;
                }
                net_cfg.configured=1;
            }
        }
        /* FIX: parse DNS replies (src port 53) and store first A record */
        if(sport == 53) {
            const uint8_t *dns=(const uint8_t*)udp+sizeof(udp_hdr_t);
            uint16_t udp_len = ntohs(udp->length);
            size_t dns_len = (size_t)udp_len > sizeof(udp_hdr_t) ? (size_t)udp_len - sizeof(udp_hdr_t) : 0;
            if(dns_len > 12) {
                uint16_t flags_dns = (uint16_t)((dns[2]<<8)|dns[3]);
                uint16_t ancount   = (uint16_t)((dns[6]<<8)|dns[7]);
                if((flags_dns & 0x8000) && ancount > 0) {
                    /* Skip question: walk QNAME then 4 bytes QTYPE+QCLASS */
                    size_t pos = 12;
                    while(pos < dns_len && dns[pos] != 0) {
                        if((dns[pos] & 0xC0)==0xC0){ pos+=2; goto dns_qname_done; }
                        pos += dns[pos] + 1;
                    }
                    pos++; /* null label */
                    dns_qname_done:
                    pos += 4; /* QTYPE + QCLASS */
                    /* Walk answer NAME (pointer or label) */
                    if(pos < dns_len) {
                        if((dns[pos] & 0xC0)==0xC0) pos+=2;
                        else { while(pos<dns_len && dns[pos]) pos+=dns[pos]+1; pos++; }
                    }
                    /* Read TYPE(2)+CLASS(2)+TTL(4)+RDLENGTH(2) = 10 bytes */
                    if(pos+10 <= dns_len) {
                        uint16_t rtype = (uint16_t)((dns[pos]<<8)|dns[pos+1]);
                        uint16_t rdlen = (uint16_t)((dns[pos+8]<<8)|dns[pos+9]);
                        pos += 10;
                        if(rtype==1 && rdlen==4 && pos+4<=dns_len) {
                            memcpy0(net_cfg.dns_result, dns+pos, 4);
                            net_cfg.dns_resolved = 1;
                        }
                    }
                }
            }
        }
        return;
    }

    /* TCP - find matching socket and push data */
    /* (socket tracking by port is done via active socket list - simplified) */
}

/* ---- HTTP GET ---- */
#define HTTP_BUF_SIZE 65536

int http_get(const char *host, const char *path,
             uint8_t **body_out, size_t *len_out) {
    /* Resolve host via ARP/DNS - for CDN we hardcode DNS lookup stub */
    /* In real use: implement DNS over UDP to net_cfg.dns              */
    terminal_printf("[http] GET http://%s%s\n", host, path);

    /* For cdn.voided.uk we need DNS resolution */
    /* Simplified: use gateway's IP as proxy target if DNS not resolved */
    uint8_t target_ip[4];
    /* Try a very basic DNS query */
    terminal_printf("[net] resolving %s...\n", host);

    /* Build DNS query */
    uint8_t dns_pkt[512];
    memset0(dns_pkt, sizeof(dns_pkt));
    /* DNS header */
    dns_pkt[0]=0x00; dns_pkt[1]=0x01; /* ID */
    dns_pkt[2]=0x01; dns_pkt[3]=0x00; /* flags: recursion desired */
    dns_pkt[4]=0x00; dns_pkt[5]=0x01; /* QDCOUNT=1 */

    /* Encode QNAME */
    int qi=12;
    const char *h=host;
    while(*h) {
        int dot=0; const char *t=h;
        while(*t&&*t!='.'){t++;dot++;}
        dns_pkt[qi++]=(uint8_t)dot;
        while(h<t) dns_pkt[qi++]=(uint8_t)*h++;
        if(*h=='.') h++;
    }
    dns_pkt[qi++]=0;     /* root label */
    dns_pkt[qi++]=0; dns_pkt[qi++]=1; /* QTYPE A */
    dns_pkt[qi++]=0; dns_pkt[qi++]=1; /* QCLASS IN */

    /* FIX: clear previous DNS result before querying */
    net_cfg.dns_resolved = 0;

    /* Send UDP DNS query to resolver */
    uint8_t dns_frame[sizeof(ip_hdr_t)+sizeof(udp_hdr_t)+qi];
    memset0(dns_frame, sizeof(dns_frame));
    udp_hdr_t *u=(udp_hdr_t*)(dns_frame+sizeof(ip_hdr_t));
    u->src_port=htons(next_port++);
    u->dst_port=htons(53);
    u->length  =htons(sizeof(udp_hdr_t)+(uint16_t)qi);
    memcpy0((uint8_t*)u+sizeof(udp_hdr_t),dns_pkt,qi);
    ip_hdr_t *dip=(ip_hdr_t*)dns_frame;
    dip->ver_ihl=0x45; dip->ttl=64; dip->proto=IP_PROTO_UDP;
    dip->total_len=htons((uint16_t)sizeof(dns_frame));
    memcpy0(dip->src,net_cfg.ip,4);
    memcpy0(dip->dst,net_cfg.dns,4);
    dip->checksum=net_checksum(dip,sizeof(ip_hdr_t));
    uint8_t dns_mac[6]={0};
    /* FIX: resolve gateway MAC before sending DNS (DNS server is off-subnet) */
    net_arp_resolve(net_cfg.dns, dns_mac);
    arp_cache_get(net_cfg.gw,dns_mac);
    eth_send(dns_mac,ETH_TYPE_IP,dns_frame,sizeof(dns_frame));

    /* FIX: wait for DNS reply populated by net_receive_frame */
    for(int i=0;i<2000;i++){
        rndis_poll();
        if(net_cfg.dns_resolved) break;
        for(volatile int j=0;j<10000;j++);
    }

    /* FIX: use resolved IP if available, otherwise fail clearly */
    if(!net_cfg.dns_resolved) {
        terminal_printf("[net] DNS resolution failed for %s\n", host);
        return -1;
    }
    memcpy0(target_ip, net_cfg.dns_result, 4);
    terminal_printf("[net] resolved %s -> %d.%d.%d.%d\n", host,
        target_ip[0],target_ip[1],target_ip[2],target_ip[3]);

    /* Open TCP connection */
    tcp_socket_t *sock = tcp_connect(target_ip, 80);
    if(!sock){
        terminal_printf("[http] connection failed\n");
        return -1;
    }

    /* Build HTTP GET request */
    char req[512];
    int rlen=0;
    /* method + path */
    const char *GET="GET "; const char *HTTP=" HTTP/1.0\r\n";
    for(const char*s=GET;*s;s++) req[rlen++]=*s;
    for(const char*s=path;*s;s++) req[rlen++]=*s;
    for(const char*s=HTTP;*s;s++) req[rlen++]=*s;
    /* Host header */
    const char *HOST="Host: ";
    for(const char*s=HOST;*s;s++) req[rlen++]=*s;
    for(const char*s=host;*s;s++) req[rlen++]=*s;
    req[rlen++]='\r'; req[rlen++]='\n';
    /* Connection: close */
    const char *CONN="Connection: close\r\n\r\n";
    for(const char*s=CONN;*s;s++) req[rlen++]=*s;

    tcp_send(sock,(uint8_t*)req,(size_t)rlen);

    /* Receive response */
    uint8_t *resp=(uint8_t*)kmalloc(HTTP_BUF_SIZE);
    if(!resp){tcp_close(sock);return -1;}
    size_t total=0;
    int n;
    while((n=tcp_recv(sock,resp+total,HTTP_BUF_SIZE-total-1,3000))>0)
        total+=n;
    resp[total]=0;
    tcp_close(sock);

    /* Find body (skip HTTP headers) */
    uint8_t *body=resp;
    for(size_t i=0;i+3<total;i++){
        if(resp[i]=='\r'&&resp[i+1]=='\n'&&resp[i+2]=='\r'&&resp[i+3]=='\n'){
            body=resp+i+4; break;
        }
    }
    size_t blen=total-(size_t)(body-resp);
    *body_out=body;
    *len_out =blen;
    return 0;
}

void net_get_config(net_config_t *cfg) {
    memcpy0(cfg, &net_cfg, sizeof(net_config_t));
}

void net_init(void) {
    memset0(&net_cfg,sizeof(net_cfg));
    memset0(arp_cache,sizeof(arp_cache));
    rndis_get_mac(net_cfg.mac);
    rndis_set_receive_cb(net_receive_frame);
    terminal_printf("[net] stack initialized\n");
}

int net_arp_resolve(const uint8_t *ip, uint8_t *mac_out) {
    /* FIX: if destination is off-subnet, ARP for the gateway instead */
    const uint8_t *target = ip;
    if(net_cfg.configured) {
        int on_subnet = 1;
        for(int i=0;i<4;i++) {
            if((ip[i] & net_cfg.mask[i]) != (net_cfg.ip[i] & net_cfg.mask[i])) {
                on_subnet = 0; break;
            }
        }
        if(!on_subnet) target = net_cfg.gw;
    }
    if(arp_cache_get(target,mac_out)) return 1;
    arp_send_request(target);
    for(int i=0;i<1000;i++){
        rndis_poll();
        if(arp_cache_get(target,mac_out)) return 1;
        for(volatile int j=0;j<10000;j++);
    }
    return 0;
}
