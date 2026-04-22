
extern uint32_t get_ticks(void);
void bin_uptime(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    uint32_t t=get_ticks(),s=t/1000,m=s/60,h=m/60;s%=60;m%=60;
    terminal_printf("up %u:%02u:%02u,  load average: 0.00 0.00 0.00\n",h,m,s);}



