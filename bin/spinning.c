#include "_bin_common.h"
void bin_spinning(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    const char*frames[]={"\\","|","/","-"};
    terminal_printf("spinning... (any key to stop)\n");
    for(int i=0;i<200;i++){
        terminal_printf("\r  %s  ",frames[i%4]);
        for(volatile int j=0;j<1000000;j++);
        if(keyboard_keyready()){keyboard_poll();break;}
    }
    terminal_putchar('\n');}