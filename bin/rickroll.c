#include "_bin_common.h"
void bin_rickroll(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_RED,VGA_COLOR_BLACK));
    terminal_printf("\n  We are no strangers to love\n  You know the rules and so do I\n  A full commitment'\''s what I'\''m thinking of\n  You wouldn'\''t get this from any other guy\n\n");
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
    terminal_printf("  (you got rick-rolled by scorpion OS)\n");}