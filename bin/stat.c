/* scorpion OS - bin/stat.c */
#include "_bin_common.h"
void bin_stat(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: stat <file>\n");return;}
    sfs_node_t*n=b_resolve(cwd,argv[1]);
    if(!n){terminal_printf("stat: %s: not found\n",argv[1]);return;}
    terminal_printf("  File: %s\n",n->name);
    terminal_printf("  Type: %s\n",n->type==SFS_DIR?"directory":"regular file");
    terminal_printf("  Size: %u bytes\n",(unsigned)n->size);
    terminal_printf("  Mode: %04o\n",n->mode?n->mode:0644);
}
