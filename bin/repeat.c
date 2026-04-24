#include "_bin_common.h"
extern void shell_exec_single(const char*);
void bin_repeat(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<3){terminal_printf("usage: repeat <n> <cmd>\n");return;}
    int n=b_atoi(argv[1]);
    for(int i=0;i<n;i++)shell_exec_single(argv[2]);}