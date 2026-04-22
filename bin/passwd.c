

void bin_passwd(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    const char*user=argc>1?argv[1]:current_user.username;
    terminal_printf("New password for %s: ",user);
    char pw[128];int i=0;char c;
    while((c=keyboard_getchar())!='\n'&&c!='\r'&&i<127){pw[i++]='*';terminal_putchar('*');}
    pw[i]=0;terminal_putchar('\n');

    auth_add_user(user,pw,current_user.uid,current_user.gid,current_user.home,current_user.shell);
    terminal_printf("password updated\n");}



