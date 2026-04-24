/* scorpion OS - bin/tac.c */
#include "_bin_common.h"
void bin_tac(char**argv,int argc,sfs_node_t*cwd){
    uint8_t*d=NULL;size_t sz=0;
    if(argc>1)b_read_file(cwd,argv[1],&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("tac: no input\n");return;}
    static size_t offs[4096];int n=0;offs[n++]=0;
    for(size_t i=0;i<sz;i++)if(d[i]=='\n'&&i+1<sz)offs[n++]=(size_t)i+1;
    for(int i=n-1;i>=0;i--){
        size_t end=(i+1<n)?offs[i+1]:sz;
        terminal_write((const char*)d+offs[i],(int)(end-offs[i]));
        if(end>0&&d[end-1]!='\n')terminal_putchar('\n');
    }
    kfree(d);
}