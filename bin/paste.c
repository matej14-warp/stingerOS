

void bin_paste(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: paste <file1> <file2>\n");return;}
    uint8_t*d1=NULL,*d2=NULL;size_t s1=0,s2=0;
    b_read_file(cwd,argv[1],&d1,&s1);b_read_file(cwd,argv[2],&d2,&s2);
    if(!d1||!d2){terminal_printf("paste: file not found\n");if(d1)kfree(d1);if(d2)kfree(d2);return;}
    size_t i=0,j=0;
    while(i<s1||j<s2){
        while(i<s1&&d1[i]!='\n')terminal_putchar((char)d1[i++]);if(i<s1)i++;
        terminal_putchar('\t');
        while(j<s2&&d2[j]!='\n')terminal_putchar((char)d2[j++]);if(j<s2)j++;
        terminal_putchar('\n');
    }
    kfree(d1);kfree(d2);
}



