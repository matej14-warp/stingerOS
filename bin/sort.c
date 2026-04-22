


void bin_sort(char**argv,int argc,sfs_node_t*cwd){
    int rev=0,uniq=0,num=0;
    const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'){for(int j=1;argv[i][j];j++){if(argv[i][j]=='r')rev=1;if(argv[i][j]=='u')uniq=1;if(argv[i][j]=='n')num=1;}}
        else file=argv[i];
    }
    uint8_t*data=NULL; size_t sz=0;
    if(file) b_read_file(cwd,file,&data,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){data=(uint8_t*)kmalloc(p->size+1);b_memcpy(data,p->data,(int)p->size);data[p->size]=0;sz=p->size;}}
    if(!data){terminal_printf("sort: no input\n");return;}
    static char lines[SORT_MAX][256];
    int n=0;
    int ls=0;
    for(size_t i=0;i<=sz&&n<SORT_MAX;i++){
        if(i==sz||data[i]=='\n'){
            int len=(int)(i-ls); if(len>255)len=255;
            b_memcpy(lines[n],(char*)data+ls,len); lines[n][len]=0;
            n++; ls=(int)i+1;
        }
    }
    kfree(data);

    for(int i=0;i<n-1;i++) for(int j=0;j<n-1-i;j++){
        int cmp;
        if(num) cmp=b_atoi(lines[j])-b_atoi(lines[j+1]);
        else cmp=b_scmp(lines[j],lines[j+1]);
        if(rev) cmp=-cmp;
        if(cmp>0){char tmp[256];b_memcpy(tmp,lines[j],256);b_memcpy(lines[j],lines[j+1],256);b_memcpy(lines[j+1],tmp,256);}
    }
    const char*prev="";
    for(int i=0;i<n;i++){
        if(uniq&&b_scmp(lines[i],prev)==0) continue;
        terminal_printf("%s\n",lines[i]);
        prev=lines[i];
    }
}




