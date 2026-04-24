#include "_bin_common.h"
#include "../drivers/net.h"
void bin_route(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    net_config_t c;net_get_config(&c);
    terminal_printf("Destination  Gateway         Genmask         Iface\n");
    terminal_printf("0.0.0.0      %d.%d.%d.%d      %d.%d.%d.%d  eth0\n",c.gw[0],c.gw[1],c.gw[2],c.gw[3],c.mask[0],c.mask[1],c.mask[2],c.mask[3]);}