
void bin_calc(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<4){terminal_printf("usage: calc <a> <op> <b>\nops: + - * / %% **\n");return;}
    long a=b_atoi(argv[1]),b2=b_atoi(argv[3]); const char*op=argv[2];
    long r=0;
    if(b_scmp(op,"+")==0)r=a+b2;
    else if(b_scmp(op,"-")==0)r=a-b2;
    else if(b_scmp(op,"*")==0)r=a*b2;
    else if(b_scmp(op,"/")==0){if(!b2){terminal_printf("calc: division by zero\n");return;}r=a/b2;}
    else if(b_scmp(op,"%")==0){if(!b2){terminal_printf("calc: division by zero\n");return;}r=a%b2;}
    else if(b_scmp(op,"**")==0){r=1;for(long i=0;i<b2;i++)r*=a;}
    else{terminal_printf("calc: unknown operator '%s'\n",op);return;}
    terminal_printf("%ld\n",r);}



