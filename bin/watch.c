#include "_bin_common.h"
extern void shell_exec_single(const char*);
extern void sleep_ms(uint32_t);
void bin_watch(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    int interval=2,start=1;
    if(argc>2&&b_scmp(argv[1],"-n")==0){interval=b_atoi(argv[2]);start=3;}
    if(start>=argc){terminal_printf("usage: watch [-n sec] <cmd>\n");return;}
    terminal_printf("Watch '%s' every %ds -- press q to stop\n",argv[start],interval);
    while(!keyboard_keyready()){
        terminal_clear();shell_exec_single(argv[start]);
        sleep_ms((uint32_t)interval*1000);
    }
    keyboard_poll();}