#include "_bin_common.h"
extern uint32_t get_ticks(void);
extern void shell_exec_single(const char*);
void bin_time(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: time <cmd>\n");return;}
    uint32_t t0=get_ticks();shell_exec_single(argv[1]);uint32_t t1=get_ticks();
    terminal_printf("\nreal %ums\n",t1-t0);}