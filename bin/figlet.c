

static const char*fig_rows(char c,int row){
    static const char*f[95][3]={
        {" "," "," "},{"|","| ","  "},{" "," "," "},{" "," "," "},{" "," "," "},
        {" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},
        {" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},
        {" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},
        {" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},
        {" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},{" "," "," "},
        {" "," "," "},{" "," "," "},{" "," "," "},
        {"
        {".
        {"
        {"..
        {"
        {"
        {"
        {"
        {"
    };
    if(c<32||c>90)return " ";
    return f[c-32][row];
}
void bin_figlet(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: figlet <text>\n");return;}
    for(int r=0;r<3;r++){
        for(int i=1;i<argc;i++){
            for(int j=0;argv[i][j];j++){
                char c=(char)b_toupper((char)argv[i][j]);
                terminal_printf("%s ",fig_rows(c,r));
            }
            if(i<argc-1)terminal_printf("  ");
        }
        terminal_putchar('\n');
    }
}



