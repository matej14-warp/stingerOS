

void bin_type(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: type <cmd>\n");return;}
    for(int i=0;bin_table[i].name;i++){
        if(b_scmp(bin_table[i].name,argv[1])==0){terminal_printf("%s is a built-in shell command\n",argv[1]);return;}
    }
    terminal_printf("%s: not found\n",argv[1]);}



