#include "_bin_common.h"
void bin_cowsay(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    const char*msg=argc>1?argv[1]:"Moo!";
    int l=b_slen(msg);
    terminal_printf(" ");for(int i=0;i<l+2;i++)terminal_putchar('-');terminal_printf("\n");
    terminal_printf("< %s >\n",msg);
    terminal_printf(" ");for(int i=0;i<l+2;i++)terminal_putchar('-');terminal_printf("\n");
    terminal_printf("        \\   ^__^\n         \\  (oo)\\_______\n            (__)\\       )\\/\\\n                ||----w |\n                ||     ||\n");}