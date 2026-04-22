

void bin_touch(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: touch <file>\n");return;}
    for(int i=1;i<argc;i++){
        sfs_node_t*n=b_resolve(cwd,argv[i]);
        if(!n) sfs_write_file(cwd,argv[i],(const uint8_t*)"",0);
    }
}




