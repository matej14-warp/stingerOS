

void bin_od(char**argv,int argc,sfs_node_t*cwd){
    if(argc<2){terminal_printf("usage: od <file>\n");return;}
    uint8_t*d=NULL;size_t sz=0;
    if(b_read_file(cwd,argv[1],&d,&sz)<0){terminal_printf("od: not found\n");return;}
    for(size_t i=0;i<sz;i+=16){
        terminal_printf("%07o ",(unsigned)i);
        for(size_t j=0;j<16&&i+j<sz;j++)terminal_printf(" %03o",(unsigned)d[i+j]);
        terminal_putchar('\n');
    }
    terminal_printf("%07o\n",(unsigned)sz);
    kfree(d);
}



