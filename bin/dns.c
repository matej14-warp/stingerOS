#include "_bin_common.h"
#include "../net/dns.h"
void bin_dns(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: dns resolve|cache|flush\n");return;}
    if(b_scmp(argv[1],"resolve")==0&&argc>2){
        uint8_t ip[4];
        if(dns_resolve(argv[2],ip)==0)terminal_printf("%s -> %d.%d.%d.%d\n",argv[2],ip[0],ip[1],ip[2],ip[3]);
        else terminal_printf("dns: failed to resolve '%s'\n",argv[2]);
    }else if(b_scmp(argv[1],"cache")==0){dns_cache_list();}
    else if(b_scmp(argv[1],"flush")==0){dns_cache_flush();}
    else terminal_printf("dns: unknown subcommand '%s'\n",argv[1]);}