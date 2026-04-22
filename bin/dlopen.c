

void bin_dlopen(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: dlopen <module>\n");return;}
    if(module_run(argv[1])!=0)terminal_printf("dlopen: failed to run '%s'\n",argv[1]);}



