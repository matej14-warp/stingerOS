#include "_bin_common.h"
void bin_matrix(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREEN,VGA_COLOR_BLACK));
    terminal_clear();
    static unsigned s=0x1337;
    for(int f=0;f<200;f++){
        s=s*1664525u+1013904223u;
        int col=(s>>8)%78,row=(s>>16)%23;
        char ch=(char)(33+(s%90));
        terminal_printf("\x1b[%d;%dH%c",row+1,col+1,ch);
        if(keyboard_keyready()){keyboard_poll();break;}
        for(volatile int i=0;i<800000;i++);
    }
    terminal_clear();terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));}