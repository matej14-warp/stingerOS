/* scorpion OS - bin/find.c */
#include "_bin_common.h"
static int smatch(const char*name,const char*pat){
    if(!pat||!pat[0]) return 1;
    if(pat[0]=='*'&&!pat[1]) return 1;
    if(pat[0]=='*'){
        for(int i=0;name[i];i++) if(smatch(name+i,pat+1)) return 1;
        return 0;
    }
    if(!name[0]) return !pat[0];
    if(pat[0]=='?'||pat[0]==name[0]) return smatch(name+1,pat+1);
    return 0;
}
static void find_r(sfs_node_t*dir,const char*base,const char*name_pat,int depth){
    if(depth>16) return;
    for(sfs_node_t*c=dir->child;c;c=c->next){
        if(!name_pat||smatch(c->name,name_pat)){
            if(base&&base[0]) terminal_printf("%s/%s\n",base,c->name);
            else terminal_printf("%s\n",c->name);
        }
        if(c->type==SFS_DIR){
            char newbase[256];
            if(base&&base[0]) terminal_printf(""),b_sncpy(newbase,base,200),newbase[b_slen(newbase)]='/',b_sncpy(newbase+b_slen(newbase),c->name,56);
            else b_sncpy(newbase,c->name,256);
            find_r(c,newbase,name_pat,depth+1);
        }
    }
}
void bin_find(char**argv,int argc,sfs_node_t*cwd){
    const char*path=".";
    const char*name_pat=NULL;
    for(int i=1;i<argc;i++){
        if(b_scmp(argv[i],"-name")==0&&i+1<argc) name_pat=argv[++i];
        else if(argv[i][0]!='-') path=argv[i];
    }
    sfs_node_t*dir=b_resolve(cwd,path);
    if(!dir||dir->type!=SFS_DIR){terminal_printf("find: %s: not a directory\n",path);return;}
    find_r(dir,"",name_pat,0);
}
