/* scorpion OS - bin/diff.c */
#include "_bin_common.h"
#define DIFF_MAX 512
void bin_diff(char**argv,int argc,sfs_node_t*cwd){
    if(argc<3){terminal_printf("usage: diff <file1> <file2>\n");return;}
    uint8_t*d1=NULL,*d2=NULL;size_t s1=0,s2=0;
    b_read_file(cwd,argv[1],&d1,&s1);b_read_file(cwd,argv[2],&d2,&s2);
    if(!d1||!d2){terminal_printf("diff: file not found\n");if(d1)kfree(d1);if(d2)kfree(d2);return;}
    static char l1[DIFF_MAX][256],l2[DIFF_MAX][256];int n1=0,n2=0,ls=0;
    for(size_t i=0;i<=s1&&n1<DIFF_MAX;i++){if(i==s1||d1[i]=='\n'){int l=(int)(i-ls);if(l>255)l=255;b_memcpy(l1[n1],(char*)d1+ls,l);l1[n1][l]=0;n1++;ls=(int)i+1;}}
    ls=0;
    for(size_t i=0;i<=s2&&n2<DIFF_MAX;i++){if(i==s2||d2[i]=='\n'){int l=(int)(i-ls);if(l>255)l=255;b_memcpy(l2[n2],(char*)d2+ls,l);l2[n2][l]=0;n2++;ls=(int)i+1;}}
    int max=n1>n2?n1:n2,diffs=0;
    for(int i=0;i<max;i++){
        const char*a=i<n1?l1[i]:"",*b=i<n2?l2[i]:"";
        if(b_scmp(a,b)!=0){terminal_printf("%dc%d\n< %s\n---\n> %s\n",i+1,i+1,a,b);diffs++;}
    }
    if(!diffs)terminal_printf("files are identical\n");
    kfree(d1);kfree(d2);
}