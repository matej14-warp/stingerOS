#include "_bin_common.h"
#include "../drivers/sound.h"
/* bin/sound.c just forwards to the driver implementation */
/* bin_sound is already defined in drivers/sound.c; provide a thin forwarder */
void bin_sound(char**argv,int argc,sfs_node_t*cwd);  /* defined in drivers/sound.c */