/* scorpion OS - bin/mkdir_p.c */
#include "_bin_common.h"
static void mkdir_recursive(sfs_node_t*root,const char*path){
    char buf[256]; b_sncpy(buf,path,256);
    char *p=buf; if(*p=='/'){p++;}
    sfs_node_t*cur=root;
    while(*p){
        char*slash=p;while(*slash&&*slash!='/')slash++;
        int wasslash=(*slash=='/');
        *slash=0;
        sfs_node_t*child=sfs_find_child(cur,p);
        if(!child) child=sfs_mkdir(cur,p);
        if(!child){terminal_printf("mkdir: failed: %s\n",p);return;}
        cur=child;
        if(wasslash)*slash='/';
        p=slash+wasslash;
    }
}
void bin_mkdir(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: mkdir <dir>\n");return;}
    int p_flag=0,start=1;
    if(argc>1&&b_scmp(argv[1],"-p")==0){p_flag=1;start=2;}
    for(int i=start;i<argc;i++){
        if(p_flag) mkdir_recursive(sfs_root(),argv[i]);
        else {
            sfs_node_t*parent=cwd;
            if(!sfs_mkdir(parent,argv[i])) terminal_printf("mkdir: failed: %s\n",argv[i]);
        }
    }
}
void bin_mkdir_p(char**argv,int argc,sfs_node_t*cwd){bin_mkdir(argv,argc,cwd);}
