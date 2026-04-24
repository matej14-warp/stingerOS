/* scorpion OS - bin/echo.c */
#include "_bin_common.h"
void bin_echo(char**argv,int argc,sfs_node_t*cwd){
    (void)cwd;
    int newline=1;
    int start=1;
    if(argc>1&&b_scmp(argv[1],"-n")==0){newline=0;start=2;}
    for(int i=start;i<argc;i++){
        terminal_writestring(argv[i]);
        if(i<argc-1) terminal_putchar(' ');
    }
    if(newline) terminal_putchar('\n');
}
