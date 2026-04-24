#include "_bin_common.h"
void bin_chmod(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: chmod <mode> <file>\n");return;}
    sfs_node_t*n=b_resolve(cwd,argv[2]);
    if(!n){terminal_printf("chmod: not found: %s\n",argv[2]);return;}
    unsigned mode=0;for(int i=0;argv[1][i];i++)mode=mode*8+(argv[1][i]-'0');
    n->mode=(uint32_t)mode;
    terminal_printf("mode set to %04o on %s\n",mode,argv[2]);}