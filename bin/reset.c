
void bin_reset(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_writestring("\x1b[2J\x1b[H\x1b[0m");terminal_clear();
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));}



