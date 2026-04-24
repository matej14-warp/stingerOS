/* scorpion OS - net/dns.c
   DNS resolver over UDP port 53.
   Builds a minimal DNS query (A record), sends via net_send_udp_raw(),
   waits for the response with net_recv_udp_raw(), and parses the answer.
   Results are cached in a small table (16 entries, LRU eviction).       */

#include "dns.h"
#include "../drivers/net.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

static void memset0(void *d,int v,int n){uint8_t*p=d;while(n--)*p++=(uint8_t)v;}
static void memcpy0(void *d,const void *s,int n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}
static int  scmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return(unsigned char)*a-(unsigned char)*b;}
static int  slen(const char *s){int n=0;while(s[n])n++;return n;}

/* ---- DNS cache ---- */
#define DNS_CACHE_SIZE 16
typedef struct {
    char    hostname[64];
    uint8_t ip[4];
    int     valid;
} dns_cache_entry_t;

static dns_cache_entry_t dns_cache[DNS_CACHE_SIZE];
static int dns_cache_next = 0;  /* LRU: next slot to evict */

static dns_cache_entry_t *cache_find(const char *host) {
    for (int i=0;i<DNS_CACHE_SIZE;i++)
        if (dns_cache[i].valid && scmp(dns_cache[i].hostname, host)==0)
            return &dns_cache[i];
    return NULL;
}

static void cache_put(const char *host, const uint8_t ip[4]) {
    int slot = dns_cache_next % DNS_CACHE_SIZE;
    dns_cache_next++;
    int hlen = slen(host);
    if (hlen > 63) hlen = 63;
    memcpy0(dns_cache[slot].hostname, host, hlen);
    dns_cache[slot].hostname[hlen] = 0;
    memcpy0(dns_cache[slot].ip, ip, 4);
    dns_cache[slot].valid = 1;
}

/* ---- DNS packet builder ---- */
/* Encodes hostname into DNS label format in buf starting at pos, returns new pos */
static int dns_encode_name(uint8_t *buf, int pos, const char *name) {
    while (*name) {
        const char *dot = name;
        while (*dot && *dot != '.') dot++;
        int len = (int)(dot - name);
        buf[pos++] = (uint8_t)len;
        for (int i=0;i<len;i++) buf[pos++] = (uint8_t)name[i];
        name = dot;
        if (*name == '.') name++;
    }
    buf[pos++] = 0;  /* root label */
    return pos;
}

void dns_init(void) {
    memset0(dns_cache, 0, sizeof(dns_cache));
    dns_cache_next = 0;
}

int dns_resolve(const char *hostname, uint8_t ip_out[4]) {
    if (!hostname || !hostname[0]) return -1;

    /* Loopback */
    if (scmp(hostname,"localhost")==0) {
        ip_out[0]=127;ip_out[1]=0;ip_out[2]=0;ip_out[3]=1;
        return 0;
    }

    /* Check if already dotted-decimal */
    int dots=0; int all_num=1;
    for (int i=0;hostname[i];i++) {
        if (hostname[i]=='.') dots++;
        else if (hostname[i]<'0'||hostname[i]>'9') { all_num=0; break; }
    }
    if (all_num && dots==3) {
        /* Parse dotted decimal */
        int oct=0; const char *p=hostname;
        while (*p && oct<4) {
            int n=0; while(*p>='0'&&*p<='9') n=n*10+(*p++)-'0';
            ip_out[oct++]=(uint8_t)n;
            if (*p=='.') p++;
        }
        return 0;
    }

    /* Cache lookup */
    dns_cache_entry_t *ce = cache_find(hostname);
    if (ce) { memcpy0(ip_out, ce->ip, 4); return 0; }

    /* Get configured DNS server */
    net_config_t cfg; net_get_config(&cfg);
    if (!cfg.configured) return -1;
    uint8_t *dns_server = cfg.dns;

    /* Build DNS query packet */
    uint8_t pkt[512];
    memset0(pkt, 0, sizeof(pkt));
    int pos = 0;
    /* Header */
    uint16_t txid = 0xAB12;
    pkt[pos++]=(uint8_t)(txid>>8); pkt[pos++]=(uint8_t)(txid&0xFF);  /* ID */
    pkt[pos++]=0x01; pkt[pos++]=0x00;  /* flags: RD */
    pkt[pos++]=0x00; pkt[pos++]=0x01;  /* QDCOUNT=1 */
    pkt[pos++]=0x00; pkt[pos++]=0x00;  /* ANCOUNT=0 */
    pkt[pos++]=0x00; pkt[pos++]=0x00;  /* NSCOUNT=0 */
    pkt[pos++]=0x00; pkt[pos++]=0x00;  /* ARCOUNT=0 */
    /* Question */
    pos = dns_encode_name(pkt, pos, hostname);
    pkt[pos++]=0x00; pkt[pos++]=0x01;  /* QTYPE=A */
    pkt[pos++]=0x00; pkt[pos++]=0x01;  /* QCLASS=IN */

    /* Send UDP to port 53 */
    if (net_send_udp_raw(dns_server, 1024, 53, pkt, (size_t)pos) < 0) return -1;

    /* Wait for response */
    uint8_t resp[512];
    int rlen = net_recv_udp_raw(1024, resp, sizeof(resp), 3000);
    if (rlen < 12) return -1;

    /* Verify transaction ID */
    if (resp[0] != pkt[0] || resp[1] != pkt[1]) return -1;
    /* Check response flag */
    if (!(resp[2] & 0x80)) return -1;  /* QR bit not set */
    uint16_t ancount = (uint16_t)((resp[6]<<8)|resp[7]);
    if (!ancount) return -1;

    /* Skip past question section */
    int rpos = 12;
    /* Skip QNAME */
    while (rpos < rlen && resp[rpos]) {
        if ((resp[rpos] & 0xC0) == 0xC0) { rpos += 2; goto skip_done; }
        rpos += resp[rpos] + 1;
    }
    rpos++;  /* null terminator */
    skip_done:
    rpos += 4;  /* QTYPE + QCLASS */

    /* Parse answer records */
    for (uint16_t a=0; a<ancount && rpos+10 < rlen; a++) {
        /* Skip name (may be pointer) */
        if ((resp[rpos] & 0xC0) == 0xC0) rpos += 2;
        else { while (rpos<rlen && resp[rpos]) rpos += resp[rpos]+1; rpos++; }
        if (rpos+10 > rlen) break;
        uint16_t rtype  = (uint16_t)((resp[rpos]<<8)|resp[rpos+1]); rpos+=2;
        /* uint16_t rclass = */ rpos+=2;
        /* uint32_t ttl   = */ rpos+=4;
        uint16_t rdlen  = (uint16_t)((resp[rpos]<<8)|resp[rpos+1]); rpos+=2;
        if (rtype == 1 && rdlen == 4 && rpos+4 <= rlen) {
            memcpy0(ip_out, resp+rpos, 4);
            cache_put(hostname, ip_out);
            return 0;
        }
        rpos += rdlen;
    }
    return -1;
}

void dns_cache_flush(void) {
    memset0(dns_cache, 0, sizeof(dns_cache));
    dns_cache_next = 0;
    terminal_printf("[dns] cache flushed\n");
}

void dns_cache_list(void) {
    int found = 0;
    for (int i=0;i<DNS_CACHE_SIZE;i++) {
        if (!dns_cache[i].valid) continue;
        terminal_printf("  %-40s  %d.%d.%d.%d\n",
            dns_cache[i].hostname,
            dns_cache[i].ip[0], dns_cache[i].ip[1],
            dns_cache[i].ip[2], dns_cache[i].ip[3]);
        found++;
    }
    if (!found) terminal_printf("  (cache empty)\n");
}
