#include "_bin_common.h"
void bin_yes(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    const char*s=argc>1?argv[1]:"y";
    for(int i=0;i<100;i++)terminal_printf("%s\n",s);}