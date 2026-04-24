#include "_bin_common.h"
#include "../net/ssh.h"
void bin_ssh(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: ssh <host> [port]\n");return;}
    int port=argc>2?b_atoi(argv[2]):22;
    terminal_printf("Connecting to %s:%d...\n",argv[1],port);
    ssh_connect(argv[1], "user", "");}