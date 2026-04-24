/* scorpion OS - bin/grep.c */
#include "_bin_common.h"
static int grep_match(const char*line,const char*pat,int ic){
    int plen=b_slen(pat);
    int llen=b_slen(line);
    for(int i=0;i<=llen-plen;i++){
        int ok=1;
        for(int j=0;j<plen;j++){
            char a=line[i+j],b2=pat[j];
            if(ic){a=b_tolower(a);b2=b_tolower(b2);}
            if(a!=b2){ok=0;break;}
        }
        if(ok) return 1;
    }
    return 0;
}
void bin_grep(char**argv,int argc,sfs_node_t*cwd){
    int inv=0,ic=0,nflag=0,cflag=0;
    const char*pat=NULL; const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'){
            for(int j=1;argv[i][j];j++){
                if(argv[i][j]=='v')inv=1;
                if(argv[i][j]=='i')ic=1;
                if(argv[i][j]=='n')nflag=1;
                if(argv[i][j]=='c')cflag=1;
            }
        } else if(!pat) pat=argv[i];
        else file=argv[i];
    }
    if(!pat){terminal_printf("usage: grep [-ivnc] <pattern> [file]\n");return;}
    /* source: file or /tmp/pipe_in */
    uint8_t*data=NULL; size_t sz=0;
    if(file) b_read_file(cwd,file,&data,&sz);
    else { sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in"); if(p&&p->data){data=(uint8_t*)kmalloc(p->size+1);b_memcpy(data,p->data,(int)p->size);data[p->size]=0;sz=p->size;} }
    if(!data){terminal_printf("grep: no input\n");return;}
    char line[512]; int li=0,lnum=0,count=0;
    for(size_t i=0;i<=sz;i++){
        if(i==sz||data[i]=='\n'){
            line[li]=0; lnum++;
            int m=grep_match(line,pat,ic);
            if(inv) m=!m;
            if(m){
                count++;
                if(!cflag){
                    if(nflag) terminal_printf("%d:",lnum);
                    terminal_printf("%s\n",line);
                }
            }
            li=0;
        } else if(li<511) line[li++]=(char)data[i];
    }
    if(cflag) terminal_printf("%d\n",count);
    kfree(data);
}
