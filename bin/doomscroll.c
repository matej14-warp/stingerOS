
static const char*posts[]={"just woke up. existential dread loading...","breaking: guy does thing. experts divided.","hot take: the kernel is mid","local man discovers bus error at 3am","poll: is segfault bussin or lowkey trash?","stinger OS user discovers happiness not in RAM","thread: i love my computer (it has never let me down) (it let me down)",NULL};
void bin_doomscroll(char**argv,int argc,sfs_node_t*cwd){(void)argv;(void)argc;(void)cwd;
    for(int i=0;posts[i];i++){terminal_printf("[post %d] %s\n",i+1,posts[i]);for(volatile int j=0;j<2000000;j++);}
}



