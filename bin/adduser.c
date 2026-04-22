

void bin_adduser(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: adduser <username>\n");return;}
    terminal_printf("Password for %s: ",argv[1]);
    char pw[128];int i=0;char c;
    while((c=keyboard_getchar())!='\n'&&c!='\r'&&i<127){pw[i++]='*';terminal_putchar('*');}
    pw[i]=0;terminal_putchar('\n');
    char home[64];b_sncpy(home,"/home/",7);b_sncpy(home+6,argv[1],57);
    if(auth_add_user(argv[1],pw,1000,1000,home,"/bin/sh")==0)
        terminal_printf("user '%s' created\n",argv[1]);
    else terminal_printf("adduser: failed\n");}



