

void bin_ls(char**argv,int argc,sfs_node_t*cwd){
    int long_fmt=0,all=0;
    const char *path=NULL;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'){for(int j=1;argv[i][j];j++){if(argv[i][j]=='l')long_fmt=1;if(argv[i][j]=='a')all=1;}}
        else path=argv[i];
    }
    sfs_node_t *dir=path?b_resolve(cwd,path):cwd;
    if(!dir){terminal_printf("ls: not found: %s\n",path);return;}
    if(dir->type!=SFS_DIR){

        if(long_fmt) terminal_printf("-rw-r--r--  1 root root %6u %s\n",(unsigned)dir->size,dir->name);
        else terminal_printf("%s\n",dir->name);
        return;
    }
    for(sfs_node_t*c=dir->child;c;c=c->next){
        if(!all&&c->name[0]=='.') continue;
        int is_dir=(c->type==SFS_DIR);
        if(long_fmt){
            terminal_printf("%s  1 root root %6u %s%s\n",
                is_dir?"drwxr-xr-x":"-rw-r--r--",(unsigned)c->size,c->name,is_dir?"/":"");
        } else {
            if(is_dir) terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_CYAN,VGA_COLOR_BLACK));
            terminal_writestring(c->name);
            if(is_dir) terminal_putchar('/');
            terminal_putchar('\n');
            terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
        }
    }
}




