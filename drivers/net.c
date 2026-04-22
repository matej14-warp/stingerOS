









static void _mz (void *d, size_t n)                { uint8_t *p=d; while(n--)*p++=0; }
static void _mc (void *d, const void *s, size_t n) { uint8_t *dd=d; const uint8_t *ss=s; while(n--)*dd++=*ss++; }
static int  _meq(const void *a, const void *b, size_t n){
    const uint8_t *aa=a,*bb=b;
    while(n--){if(*aa!=*bb)return 0;aa++;bb++;} return 1;
}

static inline uint32_t _ticks(void){ extern uint32_t get_ticks(void); return get_ticks(); }
static inline char     _kpeek(void){ extern char keyboard_peek(void);  return keyboard_peek(); }


volatile int g_net_cancel = 0;
volatile int      g_icmp_reply     = 0;
volatile uint32_t g_icmp_rtt_ticks = 0;


static void (*_poll_fn)(void)               = NULL;
static int  (*_send_fn)(const uint8_t*,size_t) = NULL;

void net_set_poll_fn(void (*fn)(void))             { _poll_fn = fn; }
void net_set_send_fn(int  (*fn)(const uint8_t*,size_t)) { _send_fn = fn; }

static void _poll(void) { if(_poll_fn) _poll_fn(); }
static void _send_frame(const uint8_t *d, size_t l) { if(_send_fn) _send_fn(d,l); }


static net_config_t _cfg;
static const uint8_t MAC_BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};



typedef struct { uint8_t ip[4]; uint8_t mac[6]; } arp_ent_t;
static arp_ent_t  _arp[ARP_CACHE];
static int        _arp_n = 0;

static void arp_put(const uint8_t *ip, const uint8_t *mac){
    for(int i=0;i<_arp_n;i++) if(_meq(_arp[i].ip,ip,4)){_mc(_arp[i].mac,mac,6);return;}
    int slot = _arp_n < ARP_CACHE ? _arp_n++ : 0;
    _mc(_arp[slot].ip, ip, 4); _mc(_arp[slot].mac, mac, 6);
}
static int arp_get(const uint8_t *ip, uint8_t *mac_out){
    for(int i=0;i<_arp_n;i++) if(_meq(_arp[i].ip,ip,4)){_mc(mac_out,_arp[i].mac,6);return 1;}
    return 0;
    }


static uint8_t _txbuf[1600];

static int eth_tx(const uint8_t *dst, uint16_t etype, const void *pay, size_t plen){
    eth_hdr_t *e = (eth_hdr_t*)_txbuf;
    _mc(e->dst, dst, 6); _mc(e->src, _cfg.mac, 6);
    e->type = htons(etype);
    _mc(_txbuf + sizeof(eth_hdr_t), pay, plen);
    _send_frame(_txbuf, sizeof(eth_hdr_t)+plen);
    return 0;
}


uint16_t net_checksum(const void *data, size_t len){
    const uint16_t *p = data; uint32_t s = 0;
    while(len>1){ s += *p++; len-=2; }
    if(len) s += *(uint8_t*)p;
    while(s>>16) s=(s&0xFFFF)+(s>>16);
    return (uint16_t)~s;
}

static uint16_t tcp_csum(const uint8_t *sip, const uint8_t *dip,
                          const uint8_t *seg, size_t slen){
    static uint8_t ps[12+2048]; if(slen>2048)slen=2048;
    _mc(ps,sip,4); _mc(ps+4,dip,4);
    ps[8]=0; ps[9]=IP_PROTO_TCP;
    ps[10]=(uint8_t)(slen>>8); ps[11]=(uint8_t)(slen&0xFF);
    _mc(ps+12,seg,slen);
    return net_checksum(ps, 12+slen);
}


static void arp_request(const uint8_t *tgt){
    arp_pkt_t a; _mz(&a,sizeof a);
    a.htype=htons(1); a.ptype=htons(0x0800); a.hlen=6; a.plen=4;
    a.oper=htons(ARP_REQUEST);
    _mc(a.sha,_cfg.mac,6); _mc(a.spa,_cfg.ip,4);
    _mc(a.tpa,tgt,4);
    eth_tx(MAC_BCAST,ETH_TYPE_ARP,&a,sizeof a);
}

static void arp_handle(const uint8_t *frame, size_t len){
    if(len < sizeof(eth_hdr_t)+sizeof(arp_pkt_t)) return;
    const arp_pkt_t *a = (const arp_pkt_t*)(frame+sizeof(eth_hdr_t));
    arp_put(a->spa, a->sha);
    if(ntohs(a->oper)==ARP_REPLY){ arp_put(a->tpa,a->tha); return; }
    if(ntohs(a->oper)==ARP_REQUEST && _meq(a->tpa,_cfg.ip,4)){
        arp_pkt_t r; _mz(&r,sizeof r);
        r.htype=htons(1); r.ptype=htons(0x0800); r.hlen=6; r.plen=4;
        r.oper=htons(ARP_REPLY);
        _mc(r.sha,_cfg.mac,6); _mc(r.spa,_cfg.ip,4);
        r.tha[0]=a->sha[0]; r.tha[1]=a->sha[1]; r.tha[2]=a->sha[2];
        r.tha[3]=a->sha[3]; r.tha[4]=a->sha[4]; r.tha[5]=a->sha[5];
        _mc(r.tpa,a->spa,4);
        eth_tx(a->sha,ETH_TYPE_ARP,&r,sizeof r);
    }
}


int net_arp_resolve(const uint8_t *ip, uint8_t *mac_out){
    if(arp_get(ip,mac_out)) return 0;
    arp_request(ip);
    uint32_t t0=_ticks();
    while((uint32_t)(_ticks()-t0)<2000){
        if(g_net_cancel) return -1;
        _poll();
        if(arp_get(ip,mac_out)) return 0;
        for(volatile int _i=0; _i<500; _i++) __asm__ volatile("pause");
    }
    return -1;
}


static const uint8_t *route_target(const uint8_t *dst){
    if(!_cfg.configured) return dst;
    for(int i=0;i<4;i++)
        if((dst[i]&_cfg.mask[i])!=(_cfg.ip[i]&_cfg.mask[i])) return _cfg.gw;
    return dst;
}





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

typedef struct {
    uint8_t offered_ip[4];
    uint8_t server_ip[4];
    uint8_t state;
    uint32_t xid;
} dhcp_state_t;

static dhcp_state_t _ds;
static dhcp_pkt_t _dhcp_pkt;







static void dhcp_send(uint8_t msgtype){
    dhcp_pkt_t *d = &_dhcp_pkt; _mz(d,sizeof *d);
    d->op=1; d->htype=1; d->hlen=6;
    d->xid=htonl(_ds.xid); d->flags=htons(0x8000);
    _mc(d->chaddr,_cfg.mac,6);
    d->magic=htonl(DHCP_MAGIC);
    int o=0;
    d->options[o++]=53; d->options[o++]=1; d->options[o++]=msgtype;
    if(msgtype==DHCP_REQUEST){
        d->options[o++]=50; d->options[o++]=4;
        _mc(&d->options[o],_ds.offered_ip,4); o+=4;
        d->options[o++]=54; d->options[o++]=4;
        _mc(&d->options[o],_ds.server_ip,4);  o+=4;
    }
    d->options[o++]=55; d->options[o++]=3;
    d->options[o++]=1; d->options[o++]=3; d->options[o++]=6;
    d->options[o++]=255;


    static uint8_t pkt[sizeof(ip_hdr_t)+sizeof(udp_hdr_t)+sizeof(dhcp_pkt_t)];
    _mz(pkt,sizeof pkt);
    udp_hdr_t *u=(udp_hdr_t*)(pkt+sizeof(ip_hdr_t));
    u->src_port=htons(DHCP_CLIENT_PORT); u->dst_port=htons(DHCP_SERVER_PORT);
    u->length=htons(sizeof(udp_hdr_t)+sizeof(dhcp_pkt_t));
    _mc(pkt+sizeof(ip_hdr_t)+sizeof(udp_hdr_t),d,sizeof *d);
    ip_hdr_t *ip=(ip_hdr_t*)pkt;
    ip->ver_ihl=0x45; ip->total_len=htons((uint16_t)sizeof(pkt));
    ip->id=htons(1); ip->ttl=64; ip->proto=IP_PROTO_UDP;

    for(int i=0;i<4;i++){ip->src[i]=0;ip->dst[i]=0xFF;}
    ip->checksum=net_checksum(ip,sizeof(ip_hdr_t));
    eth_tx(MAC_BCAST,ETH_TYPE_IP,pkt,sizeof pkt);
}


static uint8_t dhcp_parse(const dhcp_pkt_t *d, size_t dlen){
    if(dlen < 240) return 0;
    if(ntohl(d->magic) != DHCP_MAGIC) return 0;
    if(!_meq(d->chaddr,_cfg.mac,6)) return 0;
    if(ntohl(d->xid) != _ds.xid) return 0;
    uint8_t msgtype=0;
    const uint8_t *o=d->options, *end=(const uint8_t*)d + dlen;

    if(end > d->options + sizeof(d->options)) end = d->options + sizeof(d->options);
    while(o<end && *o!=255){
        if(*o==0){o++;continue;}
        uint8_t code=*o++; uint8_t olen=*o++; if(o+olen>end)break;
        switch(code){
        case 53: msgtype=o[0]; break;
        case  1: if(olen>=4) _mc(_cfg.mask,o,4); break;
        case  3: if(olen>=4) _mc(_cfg.gw,o,4);   break;
        case  6: if(olen>=4) _mc(_cfg.dns,o,4);   break;
        case 54: if(olen>=4) _mc(_ds.server_ip,o,4); break;
        }
        o+=olen;
    }
    if(msgtype==DHCP_OFFER)   _mc(_ds.offered_ip,d->yiaddr,4);
    if(msgtype==DHCP_ACK)     _mc(_cfg.ip,d->yiaddr,4);
    return msgtype;
}


static void dhcp_rx(const uint8_t *pay, size_t dlen){
    uint8_t mt = dhcp_parse((const dhcp_pkt_t*)pay, dlen);
    switch(_ds.state){
    case DHCP_ST_SELECTING:
        if(mt==DHCP_OFFER){

            _mc(_cfg.ip, _ds.offered_ip, 4);
            _ds.state=DHCP_ST_REQUESTING;
            dhcp_send(DHCP_REQUEST);
        }
        break;
    case DHCP_ST_REQUESTING:
        if(mt==DHCP_ACK){
            if(!_cfg.mask[0]){ _cfg.mask[0]=255;_cfg.mask[1]=255;_cfg.mask[2]=255;_cfg.mask[3]=0; }
            if(!_cfg.gw[0])  { _cfg.gw[0]=_cfg.ip[0];_cfg.gw[1]=_cfg.ip[1];_cfg.gw[2]=_cfg.ip[2];_cfg.gw[3]=1; }
            if(!_cfg.dns[0]) { _cfg.dns[0]=8;_cfg.dns[1]=8;_cfg.dns[2]=8;_cfg.dns[3]=8; }
            _cfg.configured=1;
            _ds.state=DHCP_ST_BOUND;
        }
        break;
    default: break;
    }
}


static void dhcp_fallback(void){
    _cfg.ip[0]=192;_cfg.ip[1]=168;_cfg.ip[2]=1;_cfg.ip[3]=100;
    _cfg.mask[0]=255;_cfg.mask[1]=255;_cfg.mask[2]=255;_cfg.mask[3]=0;
    _cfg.gw[0]=192;_cfg.gw[1]=168;_cfg.gw[2]=1;_cfg.gw[3]=1;
    _cfg.dns[0]=8;_cfg.dns[1]=8;_cfg.dns[2]=8;_cfg.dns[3]=8;
    _cfg.configured=1;
}


static int dhcp_run(uint32_t timeout_ms, int attempts){
    if(!_send_fn){
        terminal_printf("[net] ERROR: _send_fn is not set.\n");
        dhcp_fallback(); return -1;
    }
    if(!_poll_fn){
        terminal_printf("[net] ERROR: _poll_fn is not set.\n");
        dhcp_fallback(); return -1;
    }

    _mz(&_ds,sizeof _ds);
    g_net_cancel=0;


    _ds.xid = 0xDEADBEEF ^ (((uint32_t)_cfg.mac[2]<<24)|((uint32_t)_cfg.mac[3]<<16)|((uint32_t)_cfg.mac[4]<<8)|_cfg.mac[5]);

    for(int attempt=0; attempt<attempts; attempt++){
        if(g_net_cancel) break;
        _ds.state=DHCP_ST_SELECTING;
        terminal_printf("[net] DHCP discover (attempt %d/%d, timeout %dms)...\n",attempt+1,attempts,timeout_ms);
        dhcp_send(DHCP_DISCOVER);

        uint32_t t0=_ticks();
        while((uint32_t)(_ticks()-t0)<timeout_ms){
            _poll();
            if(_ds.state==DHCP_ST_BOUND){
                terminal_printf("[net] DHCP bound: %d.%d.%d.%d\n",_cfg.ip[0],_cfg.ip[1],_cfg.ip[2],_cfg.ip[3]);
                return 0;
            }
            if(g_net_cancel) break;
            if(_kpeek()==3) { g_net_cancel=1; break; }
            for(volatile int _i=0; _i<1000; _i++) __asm__ volatile("pause");
        }
    }
    if(g_net_cancel) terminal_printf("[net] DHCP cancelled\n");
    else terminal_printf("[net] DHCP failed after %d attempts — using static 192.168.1.100\n", attempts);
    dhcp_fallback();
    g_net_cancel = 0;
    return -1;
}


int net_dhcp(void)      { return dhcp_run(5000, 2); }
int net_dhcp_once(void) { return dhcp_run(3000, 1); }





typedef struct {
    uint16_t port;
    uint8_t  buf[UDP_BUFSZ];
    size_t   len;
    int      ready;
} udp_slot_t;

static udp_slot_t _udp[UDP_SLOTS];

void udp_rx_deliver(uint16_t dport, const uint8_t *data, size_t len){
    for(int i=0;i<UDP_SLOTS;i++){
        if(_udp[i].port==dport){
            size_t cp=len<UDP_BUFSZ?len:UDP_BUFSZ;
            _mc(_udp[i].buf,data,cp);
            _udp[i].len=cp; _udp[i].ready=1; return;
        }
    }
}



static tcp_socket_t *_socks[MAX_SOCKS];
static int _nsocks=0;
static uint16_t _next_port=49152;

static void sock_reg(tcp_socket_t *s){
    for(int i=0;i<MAX_SOCKS;i++) if(!_socks[i]){_socks[i]=s;_nsocks++;return;}
}
static void sock_unreg(tcp_socket_t *s){
    for(int i=0;i<MAX_SOCKS;i++) if(_socks[i]==s){_socks[i]=NULL;_nsocks--;return;}
}
static tcp_socket_t *sock_find(uint16_t lport,const uint8_t *rip,uint16_t rport){
    for(int i=0;i<MAX_SOCKS;i++){
        tcp_socket_t *s=_socks[i]; if(!s) continue;
        if(s->local_port==lport && s->remote_port==rport && _meq(s->remote_ip,rip,4)) return s;
    }
    return NULL;
}

static void tcp_tx(tcp_socket_t *s, uint8_t flags, const uint8_t *data, size_t dlen){
    uint8_t mac[6];
    const uint8_t *arp_tgt = route_target(s->remote_ip);
    if(net_arp_resolve(arp_tgt,mac)<0) return;

    size_t seg=sizeof(tcp_hdr_t)+dlen, pkt=sizeof(ip_hdr_t)+seg;
    uint8_t *buf=(uint8_t*)kmalloc(pkt); if(!buf) return;
    _mz(buf,pkt);

    tcp_hdr_t *tcp=(tcp_hdr_t*)(buf+sizeof(ip_hdr_t));
    tcp->src_port=htons(s->local_port); tcp->dst_port=htons(s->remote_port);
    tcp->seq=htonl(s->seq); tcp->ack=htonl(s->ack);
    tcp->data_offset=0x50; tcp->flags=flags; tcp->window=htons(8192);
    if(dlen) _mc((uint8_t*)tcp+sizeof(tcp_hdr_t),data,dlen);
    tcp->checksum=tcp_csum(_cfg.ip,s->remote_ip,(uint8_t*)tcp,seg);

    ip_hdr_t *ip=(ip_hdr_t*)buf;
    ip->ver_ihl=0x45; ip->total_len=htons((uint16_t)pkt);
    ip->id=htons(_next_port); ip->ttl=64; ip->proto=IP_PROTO_TCP;
    _mc(ip->src,_cfg.ip,4); _mc(ip->dst,s->remote_ip,4);
    ip->checksum=net_checksum(ip,sizeof(ip_hdr_t));

    eth_tx(mac,ETH_TYPE_IP,buf,pkt);
    kfree(buf);
}

tcp_socket_t *tcp_connect(const uint8_t *ip, uint16_t port){
    tcp_socket_t *s=(tcp_socket_t*)kzalloc(sizeof *s); if(!s) return NULL;
    _mc(s->remote_ip,ip,4);
    s->remote_port=port; s->local_port=_next_port++;
    s->seq=0xABCD1234; s->state=TCP_STATE_SYN_SENT;
    sock_reg(s);
    tcp_tx(s,TCP_SYN,NULL,0); s->seq++;
    uint32_t t0=_ticks();
    while((uint32_t)(_ticks()-t0)<5000){
        if(g_net_cancel) break;
        _poll();
        if(s->state==TCP_STATE_ESTABLISHED) return s;
        for(volatile int _i=0; _i<1000; _i++) __asm__ volatile("pause");
    }
    sock_unreg(s); kfree(s); return NULL;
}

int tcp_send(tcp_socket_t *s, const uint8_t *data, size_t len){
    if(!s||s->state!=TCP_STATE_ESTABLISHED) return -1;
    tcp_tx(s,TCP_PSH|TCP_ACK,data,len); s->seq+=(uint32_t)len;
    return (int)len;
}

int tcp_recv(tcp_socket_t *s, uint8_t *buf, size_t maxlen, uint32_t timeout_ms){
    if(!s) return -1;
    uint32_t t0=_ticks();
    while((uint32_t)(_ticks()-t0)<timeout_ms){
        if(g_net_cancel) break;
        _poll();
        if(s->rx_head!=s->rx_tail){
            size_t n=0;
            while(s->rx_head!=s->rx_tail&&n<maxlen)
                buf[n++]=s->rx_ring[(s->rx_tail++)%sizeof(s->rx_ring)];
            return (int)n;
        }
        if(s->state==TCP_STATE_FIN_WAIT||s->state==TCP_STATE_CLOSED) return 0;
        for(volatile int _i=0; _i<1000; _i++) __asm__ volatile("pause");
    }
    return 0;
}

void tcp_close(tcp_socket_t *s){
    if(!s) return;
    sock_unreg(s);
    if(s->state==TCP_STATE_ESTABLISHED) tcp_tx(s,TCP_FIN|TCP_ACK,NULL,0);
    s->state=TCP_STATE_CLOSED; kfree(s);
}


int net_send_udp_raw(const uint8_t *dst_ip, uint16_t sport, uint16_t dport,
                     const uint8_t *data, size_t len){
    uint8_t mac[6];
    const uint8_t *arp_tgt = route_target(dst_ip);
    if(net_arp_resolve(arp_tgt,mac)<0) return -1;
    size_t pktlen=sizeof(ip_hdr_t)+sizeof(udp_hdr_t)+len;
    uint8_t *pkt=(uint8_t*)kmalloc(pktlen); if(!pkt) return -1;
    _mz(pkt,pktlen);
    udp_hdr_t *u=(udp_hdr_t*)(pkt+sizeof(ip_hdr_t));
    u->src_port=htons(sport); u->dst_port=htons(dport);
    u->length=htons((uint16_t)(sizeof(udp_hdr_t)+len));
    _mc(pkt+sizeof(ip_hdr_t)+sizeof(udp_hdr_t),data,len);
    ip_hdr_t *ip=(ip_hdr_t*)pkt;
    ip->ver_ihl=0x45; ip->total_len=htons((uint16_t)pktlen);
    ip->id=htons(0xAB); ip->ttl=64; ip->proto=IP_PROTO_UDP;
    _mc(ip->src,_cfg.ip,4); _mc(ip->dst,dst_ip,4);
    ip->checksum=net_checksum(ip,sizeof(ip_hdr_t));
    eth_tx(mac,ETH_TYPE_IP,pkt,pktlen);
    kfree(pkt); return 0;
}

int net_recv_udp_raw(uint16_t sport, uint8_t *buf, size_t maxlen, uint32_t timeout_ms){
    int slot=-1;
    for(int i=0;i<UDP_SLOTS;i++) if(_udp[i].port==sport){slot=i;break;}
    if(slot<0){
        for(int i=0;i<UDP_SLOTS;i++) if(!_udp[i].port){_udp[i].port=sport;slot=i;break;}
    }
    if(slot<0) return -1;
    _udp[slot].ready=0;
    uint32_t t0=_ticks();
    while((uint32_t)(_ticks()-t0)<timeout_ms){
        if(g_net_cancel) break;
        _poll();
        if(_udp[slot].ready){
            size_t n=_udp[slot].len<maxlen?_udp[slot].len:maxlen;
            _mc(buf,_udp[slot].buf,n);
            _udp[slot].ready=0; _udp[slot].port=0;
            return (int)n;
        }
        for(volatile int _i=0; _i<1000; _i++) __asm__ volatile("pause");
    }
    _udp[slot].port=0; return 0;
}


int http_get(const char *host, const char *path, uint8_t **body_out, size_t *len_out){
    uint8_t ip[4];
    extern int dns_resolve(const char*,uint8_t[4]);
    if(dns_resolve(host,ip)!=0) return -1;
    tcp_socket_t *s=tcp_connect(ip,80); if(!s) return -1;

    static char req[512]; int rlen = 0;
    const char *method = "GET ";
    for (int i = 0; method[i]; i++) if (rlen < (int)sizeof(req)) req[rlen++] = method[i];
    for (int i = 0; path[i]; i++) if (rlen < (int)sizeof(req)) req[rlen++] = path[i];
    const char *v = " HTTP/1.0\r\nHost: ";
    for (int i = 0; v[i]; i++) if (rlen < (int)sizeof(req)) req[rlen++] = v[i];
    for (int i = 0; host[i]; i++) if (rlen < (int)sizeof(req)) req[rlen++] = host[i];
    const char *e = "\r\nConnection: close\r\n\r\n";
    for (int i = 0; e[i]; i++) if (rlen < (int)sizeof(req)) req[rlen++] = e[i];

    if (rlen >= (int)sizeof(req)) { tcp_close(s); return -1; }
    tcp_send(s, (uint8_t*)req, (size_t)rlen);

    uint8_t *resp = NULL;
    size_t total = 0, cap = 4096; uint8_t tmp[512]; int got;
    resp = (uint8_t*)kmalloc(cap); if (!resp) { tcp_close(s); return -1; }

    while((got=tcp_recv(s,tmp,sizeof tmp,3000))>0){
        if(total+(size_t)got>cap){
            uint8_t *nb=(uint8_t*)kmalloc(cap*2); if(!nb) { kfree(resp); tcp_close(s); return -1; }
            _mc(nb,resp,total); kfree(resp); resp=nb; cap*=2;
        }
        _mc(resp+total,tmp,(size_t)got); total+=(size_t)got;
    }
    tcp_close(s);

    size_t off = 0;
    for(size_t i=0;i+3<total;i++){
        if(resp[i] == '\r' && resp[i+1] == '\n' && resp[i+2] == '\r' && resp[i+3] == '\n') { off=i+4; break; }
    }
    *body_out=resp+off; *len_out=total-off;
    return 0;
}


void net_receive_frame(const uint8_t *frame, size_t len){
    if(len<sizeof(eth_hdr_t)) return;
    const eth_hdr_t *eth=(const eth_hdr_t*)frame;
    uint16_t etype=ntohs(eth->type);
    if(etype==ETH_TYPE_ARP){ arp_handle(frame,len); return; }
    if(etype!=ETH_TYPE_IP)  return;
    if(len<sizeof(eth_hdr_t)+sizeof(ip_hdr_t)) return;
    const ip_hdr_t *ip=(const ip_hdr_t*)(frame+sizeof(eth_hdr_t));
    size_t ihlen=(ip->ver_ihl&0xF)*4;

    if(ip->proto==IP_PROTO_UDP){
        if(len<sizeof(eth_hdr_t)+ihlen+sizeof(udp_hdr_t)) return;
        const uint8_t *udp_ptr = (uint8_t*)ip + ihlen;
        const udp_hdr_t *udp=(const udp_hdr_t*)udp_ptr;
        uint16_t dport=ntohs(udp->dst_port);
        size_t   uplen=ntohs(udp->length)>sizeof(udp_hdr_t)?ntohs(udp->length)-sizeof(udp_hdr_t):0;
        const uint8_t *pay=(const uint8_t*)udp+sizeof(udp_hdr_t);
        if(dport==DHCP_CLIENT_PORT) dhcp_rx(pay,uplen);
        else                        udp_rx_deliver(dport,pay,uplen);
        return;
    }

    if(ip->proto==IP_PROTO_ICMP){
        if(len<sizeof(eth_hdr_t)+ihlen+sizeof(icmp_hdr_t)) return;
        const icmp_hdr_t *icmp=(const icmp_hdr_t*)((uint8_t*)ip+ihlen);
        if(icmp->type==0){ g_icmp_reply=1; return; }
        if(icmp->type==8){
            size_t ilen=ntohs(ip->total_len)-ihlen;
            size_t plen=sizeof(ip_hdr_t)+ilen;
            uint8_t *pkt=(uint8_t*)kmalloc(plen); if(!pkt) return;
            _mc(pkt+sizeof(ip_hdr_t),(uint8_t*)ip+ihlen,ilen);
            icmp_hdr_t *ri=(icmp_hdr_t*)(pkt+sizeof(ip_hdr_t));
            ri->type=0; ri->checksum=0;
            ri->checksum=net_checksum(pkt+sizeof(ip_hdr_t),ilen);
            ip_hdr_t *rip=(ip_hdr_t*)pkt;
            rip->ver_ihl=0x45; rip->total_len=htons((uint16_t)plen);
            rip->id=htons(0x1337); rip->ttl=64; rip->proto=IP_PROTO_ICMP;
            _mc(rip->src,_cfg.ip,4); _mc(rip->dst,ip->src,4);
            rip->checksum=net_checksum(rip,sizeof(ip_hdr_t));
            uint8_t smac[6]; _mc(smac,eth->src,6);
            eth_tx(smac,ETH_TYPE_IP,pkt,plen);
            kfree(pkt);
        }
        return;
    }

    if(ip->proto==IP_PROTO_TCP){
        if(len<sizeof(eth_hdr_t)+ihlen+sizeof(tcp_hdr_t)) return;
        const tcp_hdr_t *tcp=(const tcp_hdr_t*)((uint8_t*)ip+ihlen);
        uint16_t dport=ntohs(tcp->dst_port), sport=ntohs(tcp->src_port);
        tcp_socket_t *s=sock_find(dport,ip->src,sport); if(!s) return;
        uint32_t seq_n=ntohl(tcp->seq), ack_n=ntohl(tcp->ack);
        size_t   thlen=((tcp->data_offset>>4)&0xF)*4;
        size_t   dlen=ntohs(ip->total_len)-ihlen-thlen;
        const uint8_t *dp=(const uint8_t*)tcp+thlen;
        if(s->state==TCP_STATE_SYN_SENT&&(tcp->flags&(TCP_SYN|TCP_ACK))==(TCP_SYN|TCP_ACK)){
            s->ack=seq_n+1; s->seq=ack_n; s->state=TCP_STATE_ESTABLISHED;
            tcp_tx(s,TCP_ACK,NULL,0);
        } else if(s->state==TCP_STATE_ESTABLISHED){
            if(dlen>0){
                for(size_t i=0;i<dlen;i++)
                    s->rx_ring[(s->rx_head++)%sizeof(s->rx_ring)]=dp[i];
                s->ack=seq_n+(uint32_t)dlen;
                tcp_tx(s,TCP_ACK,NULL,0);
            }
            if(tcp->flags&TCP_FIN){
                s->ack++; s->state=TCP_STATE_FIN_WAIT;
                tcp_tx(s,TCP_ACK,NULL,0);
            }
        }
    }
}


void net_init(void){
    _mz(&_cfg,sizeof _cfg);
    _mz(_arp,sizeof _arp); _arp_n=0;
    _mz(_udp,sizeof _udp);
    _mz(_socks,sizeof _socks); _nsocks=0;
    _next_port=49152;
    _mz(&_ds,sizeof _ds);
}
void net_set_mac(const uint8_t mac[6]){ _mc(_cfg.mac,mac,6); }
void net_get_config(net_config_t *out){ _mc(out,&_cfg,sizeof *out); }
void net_set_config(const net_config_t *in){ _mc(&_cfg,in,sizeof _cfg); }


const uint8_t *net_get_route(const uint8_t *dest_ip) {
    if (!_cfg.configured && !(_cfg.ip[0]|_cfg.ip[1]|_cfg.ip[2]|_cfg.ip[3]))
        return (void*)0;


    int on_link = 1;
    for (int i = 0; i < 4; i++) {
        if ((dest_ip[i] & _cfg.mask[i]) != (_cfg.ip[i] & _cfg.mask[i])) {
            on_link = 0;
            break;
        }
    }
    return on_link ? dest_ip : _cfg.gw;
}




