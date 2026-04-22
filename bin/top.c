
extern uint32_t get_ticks(void);
void bin_top(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_clear();
    terminal_printf("stinger OS top  --  uptime: %u ticks  free: %u bytes\n\n",get_ticks(),(unsigned)kmalloc_free_bytes());
    terminal_printf("  PID  %%CPU  %%MEM  CMD\n    1   0.0   2.0  kernel\n    2   0.1   0.5  shell\n\npress any key...");
    keyboard_getchar();terminal_clear();}



