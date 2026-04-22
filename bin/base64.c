

static const char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int b64dec(char c){
    if(c>='A'&&c<='Z')return c-'A';
    if(c>='a'&&c<='z')return c-'a'+26;
    if(c>='0'&&c<='9')return c-'0'+52;
    if(c=='+')return 62; if(c=='/')return 63; return -1;
}
void bin_base64(char**argv,int argc,sfs_node_t*cwd){
    int decode=0; const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(b_scmp(argv[i],"-d")==0) decode=1;
        else file=argv[i];
    }
    uint8_t*d=NULL; size_t sz=0;
    if(file) b_read_file(cwd,file,&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("base64: no input\n");return;}
    if(!decode){
        int col=0;
        for(size_t i=0;i<sz;i+=3){
            uint32_t n=(uint32_t)d[i]<<16|(i+1<sz?(uint32_t)d[i+1]<<8:0)|(i+2<sz?(uint32_t)d[i+2]:0);
            terminal_putchar(b64[(n>>18)&63]);
            terminal_putchar(b64[(n>>12)&63]);
            terminal_putchar(i+1<sz?b64[(n>>6)&63]:'=');
            terminal_putchar(i+2<sz?b64[n&63]:'=');
            col+=4; if(col>=76){terminal_putchar('\n');col=0;}
        }
        terminal_putchar('\n');
    } else {
        int state=0; uint32_t buf2=0;
        for(size_t i=0;i<sz;i++){
            int v=b64dec((char)d[i]); if(v<0) continue;
            buf2=(buf2<<6)|(uint32_t)v;
            if(++state==4){terminal_putchar((char)(buf2>>16));terminal_putchar((char)(buf2>>8));terminal_putchar((char)buf2);state=0;buf2=0;}
        }
        if(state==3){terminal_putchar((char)(buf2>>10));terminal_putchar((char)(buf2>>2));}
        else if(state==2){terminal_putchar((char)(buf2>>4));}
        terminal_putchar('\n');
    }
    kfree(d);
}




