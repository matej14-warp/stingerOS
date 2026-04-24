/* scorpion OS - net/dns.h */
#ifndef DNS_H
#define DNS_H
#include <stdint.h>

void dns_init(void);
int  dns_resolve(const char *hostname, uint8_t ip_out[4]);
void dns_cache_flush(void);
void dns_cache_list(void);

#endif /* DNS_H */
