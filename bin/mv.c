/* scorpion OS - bin/mv.c */
#include "_bin_common.h"
void bin_mv(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: mv <src> <dst>\n");return;}
    sfs_node_t*src=b_resolve(cwd,argv[1]);
    if(!src){terminal_printf("mv: not found: %s\n",argv[1]);return;}
    if(src->type==SFS_FILE){
        sfs_write_file(cwd,argv[2],src->data,src->size);
        /* Remove original — find parent */
        sfs_node_t*parent=cwd;
        if(parent->child==src)parent->child=src->next;
        else{for(sfs_node_t*p=parent->child;p;p=p->next){if(p->next==src){p->next=src->next;break;}}}
        if(src->data)kfree(src->data);
        kfree(src);
    } else {
        terminal_printf("mv: moving directories not supported\n");
    }
}
