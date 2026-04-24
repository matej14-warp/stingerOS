#include "_bin_common.h"
#include "../drivers/net.h"
void bin_dhcp(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("Sending DHCP discover...\n");
    if(net_dhcp()==0){net_config_t c;net_get_config(&c);terminal_printf("IP: %d.%d.%d.%d\n",c.ip[0],c.ip[1],c.ip[2],c.ip[3]);}
    else terminal_printf("DHCP failed\n");}