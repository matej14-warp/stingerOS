

void bin_kill(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: kill [-l] [-<sig>] <pid>\n");return;}
    if(b_scmp(argv[1],"-l")==0){terminal_printf("1)SIGHUP 2)SIGINT 3)SIGQUIT 9)SIGKILL 15)SIGTERM\n");return;}
    int sig=SIGTERM;
    int ai=1;
    if(argv[1][0]=='-'){sig=b_atoi(argv[1]+1);ai=2;}
    if(ai>=argc){terminal_printf("kill: missing pid\n");return;}
    int pid=b_atoi(argv[ai]);
    if(pid<=2)signal_raise(sig);
    terminal_printf("signal %d sent\n",sig);}



