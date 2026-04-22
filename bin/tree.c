

static void tree_r(sfs_node_t*dir,int depth,int*count_f,int*count_d){
    for(sfs_node_t*c=dir->child;c;c=c->next){
        for(int i=0;i<depth;i++) terminal_writestring("│   ");
        terminal_writestring("├── ");
        if(c->type==SFS_DIR){
            terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_CYAN,VGA_COLOR_BLACK));
            terminal_printf("%s/\n",c->name);
            terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
            (*count_d)++;
            tree_r(c,depth+1,count_f,count_d);
        } else {
            terminal_printf("%s\n",c->name);
            (*count_f)++;
        }
    }
}
void bin_tree(char**argv,int argc,sfs_node_t*cwd){
    sfs_node_t*dir=argc>1?b_resolve(cwd,argv[1]):cwd;
    if(!dir||dir->type!=SFS_DIR){terminal_printf("tree: not a directory\n");return;}
    char p[256]; sfs_path(dir,p,sizeof(p));
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_CYAN,VGA_COLOR_BLACK));
    terminal_printf("%s\n",p);
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
    int cf=0,cd=0; tree_r(dir,0,&cf,&cd);
    terminal_printf("\n%d director%s, %d file%s\n",cd,cd==1?"y":"ies",cf,cf==1?"":"s");
}




