/* scorpion OS - bin/cut.c */
#include "_bin_common.h"
void bin_cut(char**argv,int argc,sfs_node_t*cwd){
    char delim='\t'; int field=1; const char*file=NULL;
    for(int i=1;i<argc;i++){
        if(b_scmp(argv[i],"-d")==0&&i+1<argc){delim=argv[++i][0];}
        else if(b_scmp(argv[i],"-f")==0&&i+1<argc){field=b_atoi(argv[++i]);}
        else if(argv[i][0]!='-') file=argv[i];
    }
    uint8_t*d=NULL; size_t sz=0;
    if(file) b_read_file(cwd,file,&d,&sz);
    else{sfs_node_t*p=sfs_resolve(sfs_root(),"/tmp/pipe_in");if(p&&p->data){d=(uint8_t*)kmalloc(p->size+1);b_memcpy(d,p->data,(int)p->size);d[p->size]=0;sz=p->size;}}
    if(!d){terminal_printf("cut: no input\n");return;}
    int ls=0;
    for(size_t i=0;i<=sz;i++){
        if(i==sz||d[i]=='\n'){
            /* parse fields in line d[ls..i] */
            int f=1,fs=ls;
            for(size_t j=(size_t)ls;j<=i;j++){
                if(j==i||d[j]=='\n'||(char)d[j]==delim){
                    if(f==field){terminal_write((const char*)d+fs,(int)(j-fs));terminal_putchar('\n');}
                    f++; fs=(int)j+1;
                }
            }
            ls=(int)i+1;
        }
    }
    kfree(d);
}
