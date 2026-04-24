/* bin/yaoigui.c - shell command stub for launching YaoiGUI
   The actual compositor lives in shell/yaoigui.c.
   This stub is included by shell.c via bin_registry.h. */
#include "_bin_common.h"
void bin_yaoigui(char**argv, int argc, sfs_node_t *cwd) {
    (void)argv; (void)argc; (void)cwd;
    extern void __attribute__((noreturn)) yaoigui_run(void);
    yaoigui_run();
}
