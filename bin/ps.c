#include "_bin_common.h"
void bin_ps(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("  PID TTY          TIME CMD\n    1 tty0     00:00:00 kernel\n    2 tty0     00:00:00 shell\n");}