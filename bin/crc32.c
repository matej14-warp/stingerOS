/* scorpion OS - bin/crc32.c */
#include "_bin_common.h"
static uint32_t crc32_byte(uint32_t crc,uint8_t b){
    crc^=b;
    for(int i=0;i<8;i++) crc=(crc&1)?(crc>>1)^0xEDB88320u:(crc>>1);
    return crc;
}
void bin_crc32(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: crc32 <file>\n");return;}
    uint8_t*d=NULL; size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("crc32: not found: %s\n",argv[1]);return;}
    uint32_t crc=0xFFFFFFFFu;
    for(size_t i=0;i<sz;i++) crc=crc32_byte(crc,d[i]);
    crc^=0xFFFFFFFFu;
    terminal_printf("%08x  %s\n",crc,argv[1]);
    kfree(d);
}
