#include "_bin_common.h"
extern uint32_t get_ticks(void);
void bin_random(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    static uint32_t s=0;if(!s)s=get_ticks()|1;
    s=s*1664525u+1013904223u;
    int lo=argc>1?b_atoi(argv[1]):0;
    int hi=argc>2?b_atoi(argv[2]):100;
    if(hi<=lo)hi=lo+1;
    terminal_printf("%d\n",lo+(int)(s%(unsigned)(hi-lo)));}