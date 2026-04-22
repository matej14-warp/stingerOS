

void bin_nslookup(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: nslookup <host>\n");return;}
    uint8_t ip[4];
    terminal_printf("Resolving %s...\n",argv[1]);
    if(dns_resolve(argv[1],ip)==0)terminal_printf("Name:    %s\nAddress: %d.%d.%d.%d\n",argv[1],ip[0],ip[1],ip[2],ip[3]);
    else terminal_printf("nslookup: failed to resolve '%s'\n",argv[1]);}



