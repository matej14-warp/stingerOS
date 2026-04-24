#include "_bin_common.h"
extern void sleep_ms(uint32_t ms);
void bin_sleep(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: sleep <seconds>\n");return;}
    sleep_ms((uint32_t)b_atoi(argv[1])*1000);}