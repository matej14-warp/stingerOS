
void bin_uname(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    int all=0;for(int i=1;i<argc;i++)if(argv[i][0]=='-')for(int j=1;argv[i][j];j++)if(argv[i][j]=='a')all=1;
    if(all)terminal_printf("stinger stinger v4.0 i686\n");
    else terminal_printf("stinger\n");}



