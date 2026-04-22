

void bin_uniq(char**argv,int argc,sfs_node_t*cwd){
    int cflag=0;const char*file=NULL;
    for(int i=1;i<argc;i++){if(b_scmp(argv[i],"-c")==0)cflag=1;else file=argv[i];}
    uint8_t*d=NULL;size_t sz=0;
    if(file)b_read_file(cwd,file,&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("uniq: no input\n");return;}
    char prev[512]="";int count=0,ls=0;
    for(size_t i=0;i<=sz;i++){
        if(i==sz||d[i]=='\n'){
            char line[512];int len=(int)(i-ls);if(len>511)len=511;
            b_memcpy(line,(char*)d+ls,len);line[len]=0;
            if(b_scmp(line,prev)!=0){
                if(count&&cflag)terminal_printf("%7d %s\n",count,prev);
                else if(count)terminal_printf("%s\n",prev);
                b_memcpy(prev,line,len+1);count=1;
            }else count++;
            ls=(int)i+1;
        }
    }
    if(count){if(cflag)terminal_printf("%7d %s\n",count,prev);else terminal_printf("%s\n",prev);}
    kfree(d);
}



