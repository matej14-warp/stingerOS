#include "_bin_common.h"
void bin_decompress(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: decompress <in> <out>\n");return;}
    uint8_t*d=NULL;size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("decompress: not found\n");return;}
    size_t outsz=0;for(size_t i=0;i+1<sz;i+=2)outsz+=d[i];
    uint8_t*out=(uint8_t*)kmalloc(outsz+1);if(!out){kfree(d);return;}
    size_t op=0;
    for(size_t i=0;i+1<sz;i+=2)for(uint8_t j=0;j<d[i];j++)out[op++]=d[i+1];
    sfs_write_file(cwd,argv[2],out,op);
    terminal_printf("decompressed %u -> %u bytes\n",(unsigned)sz,(unsigned)op);
    kfree(d);kfree(out);}