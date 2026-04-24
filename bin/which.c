#include "_bin_common.h"
#include "../bin/bin_registry.h"
void bin_which(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: which <cmd>\n");return;}
    for(int i=0;bin_table[i].name;i++){
        if(b_scmp(bin_table[i].name,argv[1])==0){terminal_printf("/bin/%s\n",argv[1]);return;}
    }
    terminal_printf("which: %s: not found\n",argv[1]);}