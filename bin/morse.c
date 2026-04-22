
static const char*morse_tbl[]={".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."};
void bin_morse(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: morse <text>\n");return;}
    for(int i=1;i<argc;i++){
        for(int j=0;argv[i][j];j++){
            char c=b_toupper(argv[i][j]);
            if(c>='A'&&c<='Z')terminal_printf("%s ",morse_tbl[c-'A']);
            else if(c>='0'&&c<='9')terminal_printf("-----"[0]?"? ":"? ");
            else if(c==' ')terminal_printf("/ ");
        }
    }
    terminal_putchar('\n');}



