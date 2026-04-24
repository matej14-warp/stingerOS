#include "_bin_common.h"
#include "../auth/auth.h"
void bin_whoami(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("%s\n",current_user.username[0]?current_user.username:"root");}