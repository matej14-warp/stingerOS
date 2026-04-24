#include "_bin_common.h"
#include "../drivers/net.h"
#include "../fetch/fetch.h"
void bin_wget(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: wget <url>\n");return;}
    terminal_printf("Fetching: %s\n",argv[1]);
    uint8_t*body=NULL;size_t blen=0;
    /* parse host/path from url */
    const char*url=argv[1];
    if(url[0]=='h'&&url[1]=='t'&&url[2]=='t'&&url[3]=='p')url+=7;/* skip http(s):// */
    char host[128];int hi=0;
    while(url[hi]&&url[hi]!='/'&&hi<127){host[hi]=url[hi];hi++;}host[hi]=0;
    const char*path=url[hi]?url+hi:"/";
    if(http_get(host,path,&body,&blen)==0){
        /* save to filename */
        const char*fname=path;
        for(const char*p=path;*p;p++)if(*p=='/')fname=p+1;
        if(!fname[0])fname="index.html";
        sfs_write_file(cwd,fname,body,blen);
        terminal_printf("Saved %u bytes to %s\n",(unsigned)blen,fname);
        kfree(body);
    } else terminal_printf("wget: failed\n");}