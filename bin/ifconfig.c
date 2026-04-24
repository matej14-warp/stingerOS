#include "_bin_common.h"
#include "../drivers/net.h"
void bin_ifconfig(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    net_config_t cfg;net_get_config(&cfg);
    terminal_printf("eth0  Link encap:Ethernet  HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",cfg.mac[0],cfg.mac[1],cfg.mac[2],cfg.mac[3],cfg.mac[4],cfg.mac[5]);
    terminal_printf("      inet addr:%d.%d.%d.%d  Mask:%d.%d.%d.%d\n",cfg.ip[0],cfg.ip[1],cfg.ip[2],cfg.ip[3],cfg.mask[0],cfg.mask[1],cfg.mask[2],cfg.mask[3]);
    terminal_printf("      Gateway:%d.%d.%d.%d  DNS:%d.%d.%d.%d\n",cfg.gw[0],cfg.gw[1],cfg.gw[2],cfg.gw[3],cfg.dns[0],cfg.dns[1],cfg.dns[2],cfg.dns[3]);}