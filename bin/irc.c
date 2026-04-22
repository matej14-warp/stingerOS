

void bin_irc(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: irc <server> [port]\n");return;}
    int port=argc>2?b_atoi(argv[2]):6667;
    terminal_printf("Connecting to IRC %s:%d...\n",argv[1],port);
    irc_connect(argv[1], "stinger", "



