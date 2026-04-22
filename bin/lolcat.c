
void bin_lolcat(char**argv,int argc,sfs_node_t*cwd){
    uint8_t*d=NULL;size_t sz=0;
    if(argc>1)b_read_file(cwd,argv[1],&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("lolcat: no input\n");return;}
    static const uint8_t cols[]={VGA_COLOR_LIGHT_RED,VGA_COLOR_LIGHT_BROWN,VGA_COLOR_LIGHT_GREEN,VGA_COLOR_LIGHT_CYAN,VGA_COLOR_LIGHT_BLUE,VGA_COLOR_LIGHT_MAGENTA};
    int ci=0;
    for(size_t i=0;i<sz;i++){
        if(d[i]=='\n'){terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));terminal_putchar('\n');ci=0;}
        else{terminal_setcolor(terminal_make_color((vga_color_t)cols[ci%6],VGA_COLOR_BLACK));terminal_putchar((char)d[i]);ci++;}
    }
    terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));
    kfree(d);}



