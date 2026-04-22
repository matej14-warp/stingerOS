

static void rm_node(sfs_node_t*parent,sfs_node_t*node,int recursive){
    if(node->type==SFS_DIR&&recursive){
        while(node->child) rm_node(node,node->child,1);
    }
    if(node->type==SFS_DIR&&node->child){terminal_printf("rm: %s: directory not empty\n",node->name);return;}
    if(parent->child==node){parent->child=node->next;}
    else{for(sfs_node_t*p=parent->child;p;p=p->next){if(p->next==node){p->next=node->next;break;}}}
    if(node->data) kfree(node->data);
    kfree(node);
}
void bin_rm(char**argv,int argc,sfs_node_t*cwd){
    int recursive=0,force=0,start=1;
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'){
            for(int j=1;argv[i][j];j++){if(argv[i][j]=='r'||argv[i][j]=='R')recursive=1;if(argv[i][j]=='f')force=1;}
        } else {start=i;break;}
    }
    for(int i=start;i<argc;i++){
        if(argv[i][0]=='-') continue;
        sfs_node_t*n=b_resolve(cwd,argv[i]);
        if(!n){if(!force)terminal_printf("rm: %s: not found\n",argv[i]);continue;}

        char parent_path[256]; b_sncpy(parent_path,argv[i],256);
        int l=b_slen(parent_path);
        while(l>0&&parent_path[l-1]!='/')l--;
        parent_path[l>0?l-1:0]=0;
        sfs_node_t*parent=l>0?b_resolve(cwd,parent_path):cwd;
        if(!parent) parent=cwd;
        rm_node(parent,n,recursive);
    }
}




