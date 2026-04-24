/* scorpion OS - bin/tr.c */
#include "_bin_common.h"
void bin_tr(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: tr <from> <to> [file]\n");return;}
    const char*from=argv[1],*to=argv[2];
    int fl=b_slen(from),tl=b_slen(to);
    uint8_t*d=NULL; size_t sz=0;
    if(argc>3) b_read_file(cwd,argv[3],&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("tr: no input\n");return;}
    for(size_t i=0;i<sz;i++){
        char c=(char)d[i]; int found=0;
        for(int j=0;j<fl;j++){if(c==from[j]){terminal_putchar(j<tl?to[j]:to[tl-1]);found=1;break;}}
        if(!found) terminal_putchar(c);
    }
    kfree(d);
}
