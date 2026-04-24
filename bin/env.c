#include "_bin_common.h"
extern void cmd_env_cmd(void);
void bin_env_cmd_wrap(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;cmd_env_cmd();}