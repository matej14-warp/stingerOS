

void bin_yaoigui(char**argv, int argc, sfs_node_t *cwd) {
    (void)argv; (void)argc; (void)cwd;
    extern void __attribute__((noreturn)) yaoigui_run(void);
    yaoigui_run();
}




