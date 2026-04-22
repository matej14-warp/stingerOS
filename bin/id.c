

void bin_id(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    terminal_printf("uid=%u(%s) gid=%u(%s) groups=%u\n",
        current_user.uid,current_user.username[0]?current_user.username:"root",
        current_user.gid,current_user.username[0]?current_user.username:"root",
        current_user.gid);}



