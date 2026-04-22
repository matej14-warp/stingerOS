
void bin_pipes(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    static const char*segs[]={"-","|","/","\\"};
    static unsigned s=0x5EED;
    terminal_clear();
    terminal_printf("Pipes screensaver -- any key to stop\n");
    int x=40,y=12,dir=0;
    for(int f=0;f<500;f++){
        s=s*1664525u+1013904223u;
        if((s&7)==0)dir=(dir+(1+(s>>8)%3))%4;
        if(dir==0)x++;else if(dir==1)y++;else if(dir==2)x--;else y--;
        if(x<1)x=78;if(x>78)x=1;if(y<2)y=23;if(y>23)y=2;
        terminal_setcolor(terminal_make_color((vga_color_t)(1+(f/50)%7),VGA_COLOR_BLACK));
        terminal_printf("\x1b[%d;%dH%s",y,x,segs[dir]);
        for(volatile int i=0;i<500000;i++);
        if(keyboard_keyready()){keyboard_poll();break;}
    }
    terminal_clear();terminal_setcolor(terminal_make_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK));}



