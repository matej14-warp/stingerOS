





static int sh_slen_py(const char *s){ int n=0; while(s[n]) n++; return n; }
static int sh_streq_py(const char *a, const char *b){
    while(*a && *b && *a==*b){ a++; b++; } return (*a==0 && *b==0);
}


void cmd_python(char **argv, int argc){
    if(argc < 2){

        python_repl();
        return;
    }

    if(python_exec_file(argv[1]) != 0){
        terminal_printf("python: error running '%s'\n", argv[1]);
    }
}


void cmd_python_c(char **argv, int argc){
    if(argc < 3){ terminal_printf("usage: python3 -c <code>\n"); return; }

    char buf[1024]; int bi=0;
    for(int i=2; i<argc && bi<1020; i++){
        for(int j=0; argv[i][j] && bi<1020; j++) buf[bi++]=argv[i][j];
        if(i<argc-1 && bi<1020) buf[bi++]=' ';
    }
    buf[bi]=0;
    python_exec(buf);
}


void cmd_pip(char **argv, int argc){
    if(argc < 2){
        terminal_printf("Usage:\n  pip install <package>\n  pip uninstall <package>\n  pip list\n  pip show <package>\n");
        return;
    }
    if(sh_streq_py(argv[1],"install")){
        if(argc < 3){ terminal_printf("pip install: missing package name\n"); return; }
        for(int i=2; i<argc; i++) python_pip_install(argv[i]);
    } else if(sh_streq_py(argv[1],"uninstall")||sh_streq_py(argv[1],"remove")){
        if(argc < 3){ terminal_printf("pip uninstall: missing package name\n"); return; }
        for(int i=2; i<argc; i++) python_pip_remove(argv[i]);
    } else if(sh_streq_py(argv[1],"list")){
        python_pip_list();
    } else if(sh_streq_py(argv[1],"show")){
        if(argc < 3){ terminal_printf("pip show: missing package name\n"); return; }
        terminal_printf("Name: %s\nVersion: 1.0.0\nLocation: /usr/lib/python3\n", argv[2]);
    } else if(sh_streq_py(argv[1],"--version")||sh_streq_py(argv[1],"-V")){
        terminal_printf("pip 23.0.0 from /usr/lib/python3/pip (python 3.x)\n");
    } else {
        terminal_printf("pip: unknown subcommand '%s'\n", argv[1]);
        terminal_printf("  try: install, uninstall, list, show\n");
    }
}




