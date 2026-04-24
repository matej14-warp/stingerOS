/* scorpion OS - bin/wc.c */
#include "_bin_common.h"
void bin_wc(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: wc <file>\n");return;}
    uint8_t*d=NULL; size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("wc: not found: %s\n",argv[1]);return;}
    int lines=0,words=0,bytes=(int)sz,in_word=0;
    for(size_t i=0;i<sz;i++){
        if(d[i]=='\n') lines++;
        if(b_isspace((char)d[i])){in_word=0;}
        else{if(!in_word){words++;in_word=1;}}
    }
    terminal_printf("%7d %7d %7d %s\n",lines,words,bytes,argv[1]);
    kfree(d);
}
