
void bin_hexcalc(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc<2){terminal_printf("usage: hexcalc <number>\n");return;}
    unsigned long v=b_atoul(argv[1]);
    terminal_printf("dec: %lu\nhex: 0x%lx\noct: 0%lo\nbin: ",v,v,v);
    if(!v){terminal_printf("0");} else{char b[33];int i=0;unsigned long t=v;while(t){b[i++]=(char)('0'+(t&1));t>>=1;}while(i--)terminal_putchar(b[i+1]);}
    terminal_putchar('\n');}



