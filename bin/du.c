
static size_t du_r(sfs_node_t*n){if(!n)return 0;if(n->type==SFS_FILE)return n->size;size_t t=0;for(sfs_node_t*c=n->child;c;c=c->next)t+=du_r(c);return t;}
void bin_du(char**argv,int argc,sfs_node_t*cwd){
    sfs_node_t*n=argc>1?b_resolve(cwd,argv[1]):cwd;
    if(!n){terminal_printf("du: not found\n");return;}
    terminal_printf("%u\t%s\n",(unsigned)((du_r(n)+1023)/1024),argc>1?argv[1]:".");}



