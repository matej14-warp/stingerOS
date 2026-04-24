#include "_bin_common.h"
#include "../drivers/pci.h"
void bin_lspci(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;pci_list();}