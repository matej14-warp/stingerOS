/* scorpion OS - bin/ln.c */
#include "_bin_common.h"
void bin_ln(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: ln <src> <dst>\n");return;}
    sfs_node_t*src=b_resolve(cwd,argv[1]);
    if(!src||src->type!=SFS_FILE){terminal_printf("ln: source not found\n");return;}
    /* Symbolic link: copy data (no hard links in SFS) */
    sfs_write_file(cwd,argv[2],src->data,src->size);
    terminal_printf("ln: note: SFS does not support hard links; copied instead\n");
}
