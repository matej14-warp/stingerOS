

void bin_mbf(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: mbf <script.mbf>\n");return;}
    sfs_node_t*n=b_resolve(cwd,argv[1]);
    if(!n||n->type!=SFS_FILE||!n->data){terminal_printf("mbf: not found: %s\n",argv[1]);return;}
    mbf_run((const char*)n->data,n->size,cwd);}



