/* scorpion OS - bin/rev.c */
#include "_bin_common.h"
void bin_rev(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: rev <file>\n");return;}
    uint8_t*d=NULL; size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("rev: not found: %s\n",argv[1]);return;}
    /* Reverse each line */
    size_t ls=0;
    for(size_t i=0;i<=sz;i++){
        if(i==sz||d[i]=='\n'){
            for(size_t j=ls;j<i;j++) terminal_putchar((char)d[ls+(i-1-j+ls)-ls]);
            /* simpler: */
            for(size_t j=i;j>ls;j--) terminal_putchar((char)d[j-1]);
            terminal_putchar('\n');
            ls=i+1;
        }
    }
    kfree(d);
}
