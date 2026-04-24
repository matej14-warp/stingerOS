#include "_bin_common.h"
void bin_uname(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    int all=0;for(int i=1;i<argc;i++)if(argv[i][0]=='-')for(int j=1;argv[i][j];j++)if(argv[i][j]=='a')all=1;
    if(all)terminal_printf("ScorpionOS scorpion 5.0 #1 SMP i686 i686 i386 GNU/Linux\n");
    else terminal_printf("ScorpionOS\n");}