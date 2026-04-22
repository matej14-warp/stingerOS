

void bin_pwd(char**argv,int argc,sfs_node_t*cwd){
    (void)argv;(void)argc;
    char path[256]; sfs_path(cwd,path,sizeof(path));
    terminal_printf("%s\n",path);
}




