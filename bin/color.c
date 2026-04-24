#include "_bin_common.h"
void bin_color(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){
        for(int bg=0;bg<8;bg++){for(int fg=0;fg<16;fg++){terminal_setcolor(terminal_make_color((vga_color_t)fg,(vga_color_t)bg));terminal_printf(" %X%X ",fg,bg);}terminal_putchar('\n');}
        terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
        return;
    }
    int fg=b_atoi(argv[1]),bg=argc>2?b_atoi(argv[2]):0;
    terminal_setcolor(terminal_make_color((vga_color_t)(fg&0xF),(vga_color_t)(bg&0xF)));}