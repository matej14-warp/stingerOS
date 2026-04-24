/* scorpion OS - bin/toupper.c */
#include "_bin_common.h"
void bin_toupper(char**argv,int argc,sfs_node_t*cwd){
    uint8_t*d=NULL; size_t sz=0;
    if(argc>1) b_read_file(cwd,argv[1],&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("toupper: no input\n");return;}
    for(size_t i=0;i<sz;i++) terminal_putchar((char)b_toupper((char)d[i]));
    kfree(d);
}
