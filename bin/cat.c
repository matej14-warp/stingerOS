/* scorpion OS - bin/cat.c */
#include "_bin_common.h"
void bin_cat(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: cat <file>\n");return;}
    for(int i=1;i<argc;i++){
        sfs_node_t*n=b_resolve(cwd,argv[i]);
        if(!n||n->type!=SFS_FILE){terminal_printf("cat: %s: not found\n",argv[i]);continue;}
        if(n->data&&n->size) terminal_write((const char*)n->data,n->size);
        if(n->size&&((char*)n->data)[n->size-1]!='\n') terminal_putchar('\n');
    }
}
