

void bin_reboot(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("rebooting...\n");acpi_reboot();}



