
void bin_more(char**argv,int argc,sfs_node_t*cwd){
    uint8_t*d=NULL;size_t sz=0;
    if(argc>1)b_read_file(cwd,argv[1],&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("more: no input\n");return;}
    int lines=0;
    for(size_t i=0;i<sz;i++){
        terminal_putchar((char)d[i]);
        if(d[i]=='\n'&&++lines==22){
            terminal_printf("-- More -- (space=next page q=quit) ");
            char c=keyboard_getchar();terminal_putchar('\n');
            if(c=='q')break;lines=0;
        }
    }
    kfree(d);
}



