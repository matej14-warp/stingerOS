/* scorpion OS - bin/tail.c */
#include "_bin_common.h"
void bin_tail(char**argv,int argc,sfs_node_t*cwd){
    int n=10; const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'&&b_isdigit(argv[i][1])) n=b_atoi(argv[i]+1);
        else if(argv[i][0]=='-'&&argv[i][1]=='n'&&i+1<argc) n=b_atoi(argv[++i]);
        else file=argv[i];
    }
    uint8_t*d=NULL; size_t sz=0;
    if(file) b_read_file(cwd,file,&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("tail: no input\n");return;}
    /* count newlines from end */
    int lc=0; size_t start=sz;
    for(size_t i=sz;i>0;i--){if(d[i-1]=='\n'){lc++;if(lc==n){start=i;break;}}}
    terminal_write((const char*)d+start,(sz-start));
    kfree(d);
}
