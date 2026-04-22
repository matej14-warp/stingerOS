
void bin_seq(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    int first=1,last=1,step=1;
    if(argc==2)last=b_atoi(argv[1]);
    else if(argc==3){first=b_atoi(argv[1]);last=b_atoi(argv[2]);}
    else if(argc>=4){first=b_atoi(argv[1]);step=b_atoi(argv[2]);last=b_atoi(argv[3]);}
    if(!step){terminal_printf("seq: zero step\n");return;}
    for(int i=first;step>0?i<=last:i>=last;i+=step)terminal_printf("%d\n",i);}



