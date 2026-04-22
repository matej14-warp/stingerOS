
extern const char*env_get(const char*);
extern void cmd_env_cmd(void);
void bin_printenv(char**argv,int argc,sfs_node_t*cwd){(void)cwd;
    if(argc>1)terminal_printf("%s\n",env_get(argv[1]));
    else cmd_env_cmd();}



