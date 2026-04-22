







extern uint32_t get_ticks(void);
extern void     sleep_ms(uint32_t ms);


static void _dmz(void *p, size_t n){
    uint8_t *b=(uint8_t*)p; for(size_t i=0;i<n;i++) b[i]=0;
}
static void _dmc(void *dst, const void *src, size_t n){
    uint8_t *d=(uint8_t*)dst; const uint8_t *s=(const uint8_t*)src;
    for(size_t i=0;i<n;i++) d[i]=s[i];
}
static int _dmeq(const void *a, const void *b, size_t n){
    const uint8_t *x=(const uint8_t*)a, *y=(const uint8_t*)b;
    for(size_t i=0;i<n;i++) if(x[i]!=y[i]) return 0;
    return 1;
}













static const uint8_t DBCAST[4] = {255,255,255,255};


typedef struct {
    uint8_t  op,htype,hlen,hops;
    uint32_t xid;
    uint16_t secs,flags;
    uint8_t  ciaddr[4],yiaddr[4],siaddr[4],giaddr[4];
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint32_t magic;
    uint8_t  options[308];
} __attribute__((packed)) d_pkt_t;


typedef struct {
    uint8_t  ip[4], mask[4], gw[4], dns[4], server[4];
    uint32_t lease;
} d_result_t;


static size_t d_build(d_pkt_t *p, uint8_t msgtype,
                      const uint8_t mac[6],
                      const uint8_t offered[4],
                      const uint8_t srv[4])
{
    _dmz(p, sizeof *p);
    p->op    = 1;
    p->htype = 1;
    p->hlen  = 6;
    p->xid   = htonl(DXID);
    p->flags = htons(0x8000u);
    _dmc(p->chaddr, mac, 6);
    p->magic = htonl(DMAGIC);

    int o = 0;
    p->options[o++]=53; p->options[o++]=1; p->options[o++]=msgtype;

    if (msgtype == DMSG_REQUEST) {
        p->options[o++]=50; p->options[o++]=4;
        _dmc(&p->options[o], offered, 4); o+=4;
        p->options[o++]=54; p->options[o++]=4;
        _dmc(&p->options[o], srv, 4);     o+=4;
    }


    p->options[o++]=55; p->options[o++]=3;
    p->options[o++]=1; p->options[o++]=3; p->options[o++]=6;
    p->options[o++]=255;

    return (size_t)((uint8_t*)p->options - (uint8_t*)p) + (size_t)o;
}


static uint8_t d_parse(const d_pkt_t *p, size_t plen,
                        const uint8_t mac[6], d_result_t *r,
                        uint8_t srv_out[4], uint8_t offered_out[4])
{
    if (plen < sizeof(d_pkt_t))           return 0;
    if (ntohl(p->magic) != DMAGIC)        return 0;
    if (p->op != 2)                        return 0;
    if (ntohl(p->xid) != DXID)            return 0;
    if (!_dmeq(p->chaddr, mac, 6))         return 0;

    uint8_t msgtype = 0;
    const uint8_t *o = p->options, *end = o + sizeof(p->options);

    while (o < end && *o != 255) {
        if (*o == 0) { o++; continue; }
        uint8_t code = *o++; if (o>=end) break;
        uint8_t olen = *o++; if (o+olen>end) break;
        switch (code) {
        case 53: msgtype = o[0]; break;
        case  1: if(olen>=4) _dmc(r->mask,   o, 4); break;
        case  3: if(olen>=4) _dmc(r->gw,     o, 4); break;
        case  6: if(olen>=4) _dmc(r->dns,    o, 4); break;
        case 54: if(olen>=4) { _dmc(r->server, o, 4); _dmc(srv_out, o, 4); } break;
        case 51: if(olen>=4) r->lease =
                     ((uint32_t)o[0]<<24)|((uint32_t)o[1]<<16)|
                     ((uint32_t)o[2]<<8)|o[3]; break;
        }
        o += olen;
    }
    if (msgtype == DMSG_OFFER || msgtype == DMSG_ACK) {
        _dmc(offered_out, p->yiaddr, 4);
        if (msgtype == DMSG_ACK) _dmc(r->ip, p->yiaddr, 4);
    }
    return msgtype;
}


static void run_dhcp_client_new(void)
{
    d_pkt_t *tx = (d_pkt_t*)kmalloc(sizeof(d_pkt_t));
    d_pkt_t *rx = (d_pkt_t*)kmalloc(sizeof(d_pkt_t));
    if (!tx || !rx) {
        terminal_printf("[dhcp] out of memory\n");
        if (tx) kfree(tx); if (rx) kfree(rx);
        return;
    }


    net_config_t cfg; net_get_config(&cfg);
    extern volatile int g_net_cancel;
    g_net_cancel = 0;

    d_result_t   res;
    uint8_t      offered[4]={0}, srv[4]={0};
    int          attempts = 3;
    uint32_t     timeout  = 5000;

    _dmz(&res, sizeof res);

    for (int att = 0; att < attempts; att++) {
        if (g_net_cancel) break;
        terminal_printf("[dhcp] DISCOVER (attempt %d/%d)...\n", att+1, attempts);

        size_t txlen = d_build(tx, DMSG_DISCOVER, cfg.mac, NULL, NULL);
        net_send_udp_raw(DBCAST, DCLIENT_PORT, DSERVER_PORT,
                         (const uint8_t*)tx, txlen);

        uint8_t  phase = DMSG_DISCOVER;
        uint32_t t0    = get_ticks();

        while ((uint32_t)(get_ticks() - t0) < timeout) {
            if (g_net_cancel) break;
            _dmz(rx, sizeof(d_pkt_t));

            int rlen = net_recv_udp_raw(DCLIENT_PORT,
                                        (uint8_t*)rx, sizeof(d_pkt_t),
                                        200);
            if (rlen <= 0) continue;

            uint8_t mt = d_parse(rx, (size_t)rlen, cfg.mac,
                                  &res, srv, offered);

            if (phase == DMSG_DISCOVER && mt == DMSG_OFFER) {
                terminal_printf("[dhcp] OFFER from %d.%d.%d.%d"
                                " — offered %d.%d.%d.%d\n",
                    srv[0],     srv[1],     srv[2],     srv[3],
                    offered[0], offered[1], offered[2], offered[3]);

                txlen = d_build(tx, DMSG_REQUEST, cfg.mac, offered, srv);
                net_send_udp_raw(DBCAST, DCLIENT_PORT, DSERVER_PORT,
                                 (const uint8_t*)tx, txlen);
                phase = DMSG_REQUEST;
                t0    = get_ticks();
                continue;
            }

            if (phase == DMSG_REQUEST && mt == DMSG_ACK) {
                if (!res.mask[0]){ res.mask[0]=255;res.mask[1]=255;res.mask[2]=255;res.mask[3]=0; }
                if (!res.gw[0])  { res.gw[0]=res.ip[0];res.gw[1]=res.ip[1];res.gw[2]=res.ip[2];res.gw[3]=1; }
                if (!res.dns[0]) { res.dns[0]=8;res.dns[1]=8;res.dns[2]=8;res.dns[3]=8; }
                if (!res.lease)    res.lease = 86400;

                terminal_printf("[dhcp] bound: %d.%d.%d.%d/%d.%d.%d.%d"
                                " gw %d.%d.%d.%d dns %d.%d.%d.%d lease %us\n",
                    res.ip[0],   res.ip[1],   res.ip[2],   res.ip[3],
                    res.mask[0], res.mask[1], res.mask[2], res.mask[3],
                    res.gw[0],   res.gw[1],   res.gw[2],   res.gw[3],
                    res.dns[0],  res.dns[1],  res.dns[2],  res.dns[3],
                    res.lease);


                net_config_t nc;
                _dmc(nc.ip,  res.ip,  4);
                _dmc(nc.mask,res.mask,4);
                _dmc(nc.gw,  res.gw,  4);
                _dmc(nc.dns, res.dns, 4);
                _dmc(nc.mac, cfg.mac, 6);
                nc.configured = 1;
                nc.dns_resolved = 0;
                net_set_config(&nc);

                g_net_cancel = 0;
                kfree(tx); kfree(rx);
                return;
            }

            if (phase == DMSG_REQUEST && mt == DMSG_NAK) {
                terminal_printf("[dhcp] NAK — retrying\n");
                break;
            }
        }
        if (g_net_cancel) break;
        terminal_printf("[dhcp] timeout on attempt %d\n", att+1);
    }

    if (g_net_cancel) terminal_printf("[dhcp] cancelled\n");
    else terminal_printf("[dhcp] failed — no lease obtained\n");

    g_net_cancel = 0;
    kfree(tx); kfree(rx);
}




