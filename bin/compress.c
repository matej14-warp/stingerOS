#include "_bin_common.h"
/* Simple RLE compression: [count][byte] pairs */
void bin_compress(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: compress <in> <out>\n");return;}
    uint8_t*d=NULL;size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("compress: not found\n");return;}
    uint8_t*out=(uint8_t*)kmalloc(sz*2+4);if(!out){kfree(d);return;}
    size_t op=0;
    for(size_t i=0;i<sz;){
        uint8_t c=d[i];uint8_t cnt=1;
        while(i+cnt<sz&&d[i+cnt]==c&&cnt<255)cnt++;
        out[op++]=cnt;out[op++]=c;i+=cnt;
    }
    sfs_write_file(cwd,argv[2],out,op);
    terminal_printf("compressed %u -> %u bytes\n",(unsigned)sz,(unsigned)op);
    kfree(d);kfree(out);}