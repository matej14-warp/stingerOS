

void bin_cp(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: cp <src> <dst>\n");return;}
    sfs_node_t*src=b_resolve(cwd,argv[1]);
    if(!src||src->type!=SFS_FILE){terminal_printf("cp: source not found: %s\n",argv[1]);return;}
    sfs_write_file(cwd,argv[2],src->data,src->size);
}




