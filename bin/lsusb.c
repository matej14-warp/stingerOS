#include "_bin_common.h"
#include "../drivers/usb.h"
void bin_lsusb(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;usb_list_devices();}