

void bin_tee(char**argv,int argc,sfs_node_t*cwd){

    if(argc<2){terminal_printf("usage: tee <file>\n");return;}
    sfs_node_t*pipe=sfs_resolve(sfs_root(),"/tmp/pipe_in");
    if(!pipe||!pipe->data){terminal_printf("tee: no pipe input\n");return;}
    terminal_write((const char*)pipe->data,pipe->size);
    sfs_write_file(cwd,argv[1],pipe->data,pipe->size);
}




