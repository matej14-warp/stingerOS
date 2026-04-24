#include "_bin_common.h"
void bin_ascii(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("Dec  Hex  Chr    Dec  Hex  Chr    Dec  Hex  Chr    Dec  Hex  Chr\n");
    for(int i=32;i<128;i+=4){
        for(int j=0;j<4&&i+j<128;j++){
            char c=(char)(i+j);
            terminal_printf(" %3d  %02x   %c    ",i+j,i+j,(c>=32&&c<127)?c:'.');
        }
        terminal_putchar('\n');
    }
}