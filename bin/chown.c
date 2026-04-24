#include "_bin_common.h"
void bin_chown(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: chown <user> <file>\n");return;}
    sfs_node_t*n=b_resolve(cwd,argv[2]);
    if(!n){terminal_printf("chown: not found: %s\n",argv[2]);return;}
    terminal_printf("chown: ownership change not stored in SFS (no-op)\n");}