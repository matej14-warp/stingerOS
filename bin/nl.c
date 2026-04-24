/* scorpion OS - bin/nl.c */
#include "_bin_common.h"
void bin_nl(char**argv,int argc,sfs_node_t*cwd){
    uint8_t*d=NULL; size_t sz=0;
    if(argc>1) b_read_file(cwd,argv[1],&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("nl: no input\n");return;}
    int n=1,ls=0;
    for(size_t i=0;i<=sz;i++){
        if(i==sz||d[i]=='\n'){
            terminal_printf("%6d\t",n++);
            terminal_write((const char*)d+ls,(int)(i-ls));
            terminal_putchar('\n');
            ls=(int)i+1;
        }
    }
    kfree(d);
}
