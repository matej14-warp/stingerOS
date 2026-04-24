/* scorpion OS - bin/strings.c */
#include "_bin_common.h"
void bin_strings(char**argv,int argc,sfs_node_t*cwd){
    int minlen=4;
    const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'&&argv[i][1]=='n'&&i+1<argc) minlen=b_atoi(argv[++i]);
        else file=argv[i];
    }
    if(!file){terminal_printf("usage: strings <file>\n");return;}
    uint8_t*d=NULL; size_t sz=0;
    if(b_read_file(cwd,file,&d,&sz)<0){terminal_printf("strings: not found: %s\n",file);return;}
    char buf[256]; int bi=0;
    for(size_t i=0;i<=sz;i++){
        uint8_t c=(i<sz)?d[i]:0;
        if(c>=32&&c<127&&bi<255) buf[bi++]=(char)c;
        else { if(bi>=minlen){buf[bi]=0;terminal_printf("%s\n",buf);} bi=0; }
    }
    kfree(d);
}
