

void bin_head(char**argv,int argc,sfs_node_t*cwd){
    int n=10; const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'&&b_isdigit(argv[i][1])) n=b_atoi(argv[i]+1);
        else if(argv[i][0]=='-'&&argv[i][1]=='n'&&i+1<argc) n=b_atoi(argv[++i]);
        else file=argv[i];
    }
    uint8_t*d=NULL; size_t sz=0;
    if(file) b_read_file(cwd,file,&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("head: no input\n");return;}
    int lines=0; for(size_t i=0;i<sz&&lines<n;i++){terminal_putchar((char)d[i]);if(d[i]=='\n')lines++;}
    kfree(d);
}




