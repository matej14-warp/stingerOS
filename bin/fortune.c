
static const char*fortunes[]={"Life is short; use a fast bootloader.","There is no place like 127.0.0.1.","sudo make me a sandwich.","rm -rf / is not a solution.","Have you tried turning it off and on again?","Real programmers count from 0.","It works on my machine.","There are 10 types of people: those who understand binary and those who don'\''t.","The best code is no code at all.","It'\''s not a bug, it'\''s a feature.",NULL};
static unsigned seed=0xCAFEBABE;
void bin_fortune(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    seed=seed*1664525u+1013904223u;int n=0;while(fortunes[n])n++;
    terminal_printf("%s\n",fortunes[seed%n]);}



