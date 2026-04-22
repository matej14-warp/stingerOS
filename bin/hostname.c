

void bin_hostname(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc>1){auth_set_hostname(argv[1]);terminal_printf("hostname set to: %s\n",argv[1]);}
    else{char h[64];auth_get_hostname(h,64);terminal_printf("%s\n",h);}}



