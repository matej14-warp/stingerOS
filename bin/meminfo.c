#include "_bin_common.h"
void bin_meminfo(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    size_t fr=kmalloc_free_bytes(),tot=8*1024*1024;
    terminal_printf("MemTotal:%8u kB\nMemFree: %8u kB\nMemUsed: %8u kB\n",(unsigned)(tot/1024),(unsigned)(fr/1024),(unsigned)((tot-fr)/1024));}