/* scorpion OS - bin/append.c */
#include "_bin_common.h"
void bin_append(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: append <text> <file>\n");return;}
    sfs_node_t*n=b_resolve(cwd,argv[argc-1]);
    size_t existing=n&&n->type==SFS_FILE?n->size:0;
    size_t addlen=0;
    for(int i=1;i<argc-1;i++) addlen+=b_slen(argv[i])+(i<argc-2?1:0);
    uint8_t*buf=(uint8_t*)kmalloc(existing+addlen+2);
    if(!buf)return;
    if(existing&&n->data) b_memcpy(buf,n->data,(int)existing);
    size_t pos=existing;
    for(int i=1;i<argc-1;i++){
        int l=b_slen(argv[i]);
        b_memcpy(buf+pos,argv[i],l); pos+=l;
        if(i<argc-2) buf[pos++]=' ';
    }
    buf[pos++]='\n';
    sfs_write_file(cwd,argv[argc-1],buf,pos);
    kfree(buf);
}
