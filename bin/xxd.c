/* scorpion OS - bin/xxd.c */
#include "_bin_common.h"
void bin_xxd(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: xxd <file>\n");return;}
    uint8_t*d=NULL; size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("xxd: not found: %s\n",argv[1]);return;}
    for(size_t i=0;i<sz;i+=16){
        terminal_printf("%08x: ",(unsigned)i);
        for(size_t j=0;j<16;j++){
            if(i+j<sz) terminal_printf("%02x",d[i+j]);
            else terminal_printf("  ");
            if(j%2==1) terminal_putchar(' ');
        }
        terminal_printf(" ");
        for(size_t j=0;j<16&&i+j<sz;j++) terminal_putchar((d[i+j]>=32&&d[i+j]<127)?(char)d[i+j]:'.');
        terminal_putchar('\n');
    }
    kfree(d);
}
