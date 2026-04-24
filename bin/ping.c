/* scorpion OS - bin/ping.c
   ICMP echo ping with proper timer, retry loop, and DNS resolution.
   Uses the net layer's g_icmp_reply flag set by net_receive_frame.    */
#include "_bin_common.h"
#include "../drivers/net.h"

/* Exposed by net.c — set when an ICMP echo reply arrives */
extern volatile int      g_icmp_reply;
extern volatile uint32_t g_icmp_rtt_ticks;  /* ticks at which reply arrived */
/* Exposed by kernel.c */
extern uint32_t get_ticks(void);
/* Exposed by net.c */
extern void (*g_net_poll_fn_pub)(void);

static void do_poll(void) {
    /* net_poll_fn is static; call via the public wrapper */
    extern void net_poll_pub(void);
    net_poll_pub();
}

void bin_ping(char**argv, int argc, sfs_node_t*cwd){
    (void)cwd;
    int count = 4;
    const char *host = NULL;

    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'&&argv[i][1]=='c'&&i+1<argc){
            i++; count=0;
            for(int j=0;argv[i][j]>='0'&&argv[i][j]<='9';j++)
                count=count*10+(argv[i][j]-'0');
            if(count<1)count=1; if(count>20)count=20;
        } else { host=argv[i]; }
    }
    if(!host){terminal_printf("usage: ping [-c count] <ip|hostname>\n");return;}

    uint8_t ip[4]={0,0,0,0};

    /* loopback special case */
    int is_lo = (b_scmp(host,"localhost")==0||b_scmp(host,"127.0.0.1")==0);
    if(is_lo){ ip[0]=127;ip[1]=0;ip[2]=0;ip[3]=1; }
    else {
        /* try dotted decimal */
        int is_num=1;
        for(int i=0;host[i];i++) if(host[i]!='.'&&(host[i]<'0'||host[i]>'9')){is_num=0;break;}
        if(is_num){
            int oct=0; const char*p=host;
            while(*p&&oct<4){int n=0;while(*p>='0'&&*p<='9')n=n*10+(*p++)-'0';ip[oct++]=(uint8_t)n;if(*p=='.')p++;}
        } else {
            terminal_printf("Resolving %s...\n",host);
            /* Use net.c's DNS resolution via dns module */
            extern int dns_resolve(const char*, uint8_t*);
            if(dns_resolve(host,ip)!=0){
                terminal_printf("ping: cannot resolve '%s'\n",host);
                return;
            }
            terminal_printf("Resolved: %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
        }
    }

    terminal_printf("PING %s (%d.%d.%d.%d): %d data bytes\n",
        host,ip[0],ip[1],ip[2],ip[3],count);

    if(is_lo){
        for(int i=1;i<=count;i++)
            terminal_printf("64 bytes from 127.0.0.1: icmp_seq=%d ttl=64 time<1ms\n",i);
        terminal_printf("\n--- %s ping statistics ---\n%d sent, %d received, 0%% loss\n",host,count,count);
        return;
    }

    net_config_t cfg; net_get_config(&cfg);

    /* Resolve MAC for next hop */
    uint8_t dst_mac[6];
    if(!net_arp_resolve(ip, dst_mac)){
        terminal_printf("ping: no route to host %d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
        return;
    }

    int sent=0,recv=0;

    for(int seq=1;seq<=count;seq++){
        /* Build ICMP echo request inside IP/Ethernet frame */
        uint8_t pkt[74];
        for(int i=0;i<74;i++) pkt[i]=0;

        /* Ethernet header */
        eth_hdr_t *eth=(eth_hdr_t*)pkt;
        for(int i=0;i<6;i++) eth->dst[i]=dst_mac[i];
        for(int i=0;i<6;i++) eth->src[i]=cfg.mac[i];
        eth->type=htons(ETH_TYPE_IP);

        /* IP header */
        ip_hdr_t *iph=(ip_hdr_t*)(pkt+14);
        iph->ver_ihl=0x45; iph->dscp=0;
        iph->total_len=htons(52); /* 20 IP + 8 ICMP + 24 data */
        iph->id=htons((uint16_t)(0x1000+seq));
        iph->flags_frag=0; iph->ttl=64; iph->proto=IP_PROTO_ICMP;
        for(int i=0;i<4;i++) iph->src[i]=cfg.ip[i];
        for(int i=0;i<4;i++) iph->dst[i]=ip[i];
        iph->checksum=net_checksum(iph,20);

        /* ICMP echo request */
        icmp_hdr_t *ich=(icmp_hdr_t*)(pkt+14+20);
        uint8_t *icmpdata=pkt+14+20+8;
        for(int i=0;i<24;i++) icmpdata[i]=(uint8_t)(i+0x10);

        ich->type=8; ich->code=0;
        ich->id=htons(0xABCD);
        ich->seq=htons((uint16_t)seq);
        ich->checksum=0;
        /* checksum covers ICMP header + data */
        uint8_t icmp_buf[32];
        for(int i=0;i<8;i++) icmp_buf[i]=((uint8_t*)ich)[i];
        for(int i=0;i<24;i++) icmp_buf[8+i]=icmpdata[i];
        ich->checksum=net_checksum(icmp_buf,32);

        /* Clear reply flag before sending */
        g_icmp_reply=0;

        /* Send frame via net_send directly */
        extern int net_send_raw_frame(const uint8_t*,size_t);
        net_send_raw_frame(pkt,74);
        sent++;

        /* Wait up to 1000ms for reply, polling the net driver */
        uint32_t t0=get_ticks();
        int got=0;
        while((uint32_t)(get_ticks()-t0)<1000){
            do_poll();
            if(g_icmp_reply){
                uint32_t rtt=(uint32_t)(get_ticks()-t0);
                terminal_printf("64 bytes from %d.%d.%d.%d: icmp_seq=%d ttl=64 time=%ums\n",
                    ip[0],ip[1],ip[2],ip[3],seq,rtt);
                g_icmp_reply=0; recv++; got=1; break;
            }
            for(volatile int w=0;w<500;w++);
        }
        if(!got) terminal_printf("Request timeout for icmp_seq %d\n",seq);

        /* 1 second gap between pings */
        if(seq<count){
            uint32_t t1=get_ticks();
            while((uint32_t)(get_ticks()-t1)<1000){
                do_poll();
                for(volatile int w=0;w<500;w++);
            }
        }
    }

    int loss=sent>0?(sent-recv)*100/sent:100;
    terminal_printf("\n--- %s ping statistics ---\n%d sent, %d received, %d%% packet loss\n",
        host,sent,recv,loss);
}
