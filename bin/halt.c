#include "_bin_common.h"
#include "../kernel/acpi.h"
void bin_halt(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("shutting down...\n");acpi_poweroff();}