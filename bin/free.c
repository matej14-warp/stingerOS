#include "_bin_common.h"
void bin_free(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    size_t fr=kmalloc_free_bytes(),tot=8*1024*1024;
    terminal_printf("              total        used        free\nMem:    %10u  %10u  %10u\n",(unsigned)tot,(unsigned)(tot-fr),(unsigned)fr);}