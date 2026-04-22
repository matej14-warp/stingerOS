
void bin_banner(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: banner <text>\n");return;}

    for(int r=0;r<3;r++){
        for(int ci=1;ci<argc;ci++){
            for(int j=0;argv[ci][j];j++){
                char c=argv[ci][j];
                if(c>='a'&&c<='z')c=(char)(c-32);
                if(r==0)terminal_printf("
                else if(r==1)terminal_printf("
                else terminal_printf("
                terminal_putchar(' ');
            }
        }
        terminal_putchar('\n');
    }
}



