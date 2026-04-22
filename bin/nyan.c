
void bin_nyan(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    static const char*colors[]={"\x1b[31m","\x1b[33m","\x1b[32m","\x1b[36m","\x1b[34m","\x1b[35m"};
    terminal_clear();
    for(int f=0;f<40;f++){
        terminal_printf("\x1b[H");
        for(int r=0;r<12;r++){
            for(int c=0;c<60;c++)terminal_printf("%s+\x1b[0m",colors[(c+r+f)%6]);
            terminal_putchar('\n');
        }
        terminal_printf("\x1b[0m  ~(=^.^=)~  nyan nyan nyan  frame %d  (any key to stop)\n",f);
        if(keyboard_keyready()){keyboard_poll();break;}
        for(volatile int i=0;i<5000000;i++);
    }
    terminal_clear();}



