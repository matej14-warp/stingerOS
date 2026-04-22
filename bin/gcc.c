








static int _slen(const char *s){ int n=0; while(*s++)n++; return n; }
static int _scmp(const char *a,const char *b){ while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
static int _sncmp(const char *a,const char *b,int n){ while(n--&&*a&&*a==*b){a++;b++;} return n<0?0:(unsigned char)*a-(unsigned char)*b; }
static int _sisspace(char c){ return c==' '||c=='\t'||c=='\r'; }
static int _ends_with(const char *s,const char *suf){
    int sl=_slen(s),tl=_slen(suf);
    if(sl<tl)return 0;
    return _scmp(s+sl-tl,suf)==0;
}


static int gcc_check(const char *src, size_t len, const char *fname){
    int errors=0;
    int brace_depth=0;
    int paren_depth=0;
    int in_string=0;
    int in_char=0;
    int in_line_comment=0;
    int in_block_comment=0;
    int line=1;
    int line_has_stmt=0;

    for(size_t i=0;i<len;i++){
        char c=src[i];


        if(c=='\n'){
            in_line_comment=0;
            line++;
            line_has_stmt=0;
            continue;
        }

        if(in_line_comment) continue;

        if(in_block_comment){
            if(c=='*'&&i+1<len&&src[i+1]=='/'){in_block_comment=0;i++;}
            continue;
        }

        if(!in_string&&!in_char){
            if(c=='/'&&i+1<len&&src[i+1]=='/'){in_line_comment=1;i++;continue;}
            if(c=='/'&&i+1<len&&src[i+1]=='*'){in_block_comment=1;i++;continue;}
        }

        if(in_string){
            if(c=='\\'){i++;continue;}
            if(c=='"'){in_string=0;}
            continue;
        }
        if(in_char){
            if(c=='\\'){i++;continue;}
            if(c=='\''){in_char=0;}
            continue;
        }

        if(c=='"'){in_string=1;continue;}
        if(c=='\''){in_char=1;continue;}

        if(c=='{'){brace_depth++;continue;}
        if(c=='}'){
            brace_depth--;
            if(brace_depth<0){
                terminal_printf("%s:%d: error: unexpected '}'\n",fname,line);
                errors++;brace_depth=0;
            }
            continue;
        }
        if(c=='('){paren_depth++;continue;}
        if(c==')'){
            paren_depth--;
            if(paren_depth<0){
                terminal_printf("%s:%d: error: unexpected ')'\n",fname,line);
                errors++;paren_depth=0;
            }
            continue;
        }
    }

    if(in_string){
        terminal_printf("%s: error: unterminated string literal\n",fname);
        errors++;
    }
    if(in_block_comment){
        terminal_printf("%s: error: unterminated block comment\n",fname);
        errors++;
    }
    if(brace_depth>0){
        terminal_printf("%s: error: %d unclosed '{'\n",fname,brace_depth);
        errors++;
    }


    for(size_t i=0;i+8<len;i++){
        if(src[i]=='

            size_t j=i+1;
            while(j<len&&_sisspace(src[j]))j++;
            if(_sncmp(src+j,"include",7)==0){
                j+=7;
                while(j<len&&_sisspace(src[j]))j++;
                if(j<len&&src[j]!='"'&&src[j]!='<'){

                    int ln=1; for(size_t k=0;k<i;k++) if(src[k]=='\n')ln++;
                    terminal_printf("%s:%d: error:
                    errors++;
                }
            }
        }
    }

    return errors;
}


static int count_lines(const char *src, size_t len){
    int n=1; for(size_t i=0;i<len;i++) if(src[i]=='\n')n++; return n;
}


void bin_gcc(char **argv, int argc, sfs_node_t *cwd){

    if(argc>=2&&_scmp(argv[1],"--version")==0){
        terminal_printf("stinger-gcc (stinger hosted) 1.0.0\n");
        terminal_printf("Target: i686-stinger-elf\n");
        terminal_printf("Based on GCC infrastructure concepts; no backend codegen yet.\n");
        terminal_printf("Use the host cross-compiler for real compilation.\n");
        return;
    }

    if(argc<2||_scmp(argv[1],"--help")==0||_scmp(argv[1],"-h")==0){
        terminal_printf("Usage: gcc <file.c> [-o output] [-v]\n");
        terminal_printf("       gcc --version\n");
        terminal_printf("Options:\n");
        terminal_printf("  -o <output>   specify output file name\n");
        terminal_printf("  -v            verbose output\n");
        terminal_printf("  --version     show version\n");
        terminal_printf("  --help        show this help\n");
        terminal_printf("\nNote: stinger gcc performs syntax checking and creates\n");
        terminal_printf("SFS output markers. Real codegen uses the host cross-compiler.\n");
        return;
    }


    const char *infile=NULL, *outfile=NULL;
    int verbose=0;
    for(int i=1;i<argc;i++){
        if(_scmp(argv[i],"-o")==0&&i+1<argc){outfile=argv[++i];continue;}
        if(_scmp(argv[i],"-v")==0){verbose=1;continue;}
        if(argv[i][0]=='-'){

            continue;
        }
        if(!infile) infile=argv[i];
    }

    if(!infile){
        terminal_printf("gcc: error: no input files\n");
        return;
    }


    if(!_ends_with(infile,".c")){
        terminal_printf("gcc: error: '%s' is not a C source file\n",infile);
        return;
    }


    sfs_node_t *src_node=sfs_resolve(cwd,infile);
    if(!src_node||src_node->type!=SFS_FILE){
        terminal_printf("gcc: error: '%s': No such file\n",infile);
        return;
    }
    if(!src_node->data||src_node->size==0){
        terminal_printf("gcc: error: '%s' is empty\n",infile);
        return;
    }

    const char *src=(const char*)src_node->data;
    size_t slen=src_node->size;
    int lines=count_lines(src,slen);

    if(verbose){
        terminal_printf("gcc: reading '%s' (%d bytes, %d lines)\n",infile,(int)slen,lines);
    }

    terminal_printf("gcc: checking '%s'...\n",infile);
    int errors=gcc_check(src,slen,infile);

    if(errors>0){
        terminal_printf("gcc: %d error(s) — compilation failed\n",errors);
        return;
    }

    terminal_printf("gcc: '%s': no errors found\n",infile);


    char outname[128];
    if(outfile){
        int i=0;while(outfile[i]&&i<127){outname[i]=outfile[i];i++;}outname[i]=0;
    } else {

        int i=0;while(infile[i]&&infile[i]!='.'&&i<120){outname[i]=infile[i];i++;}
        outname[i]='.';outname[i+1]='e';outname[i+2]='l';outname[i+3]='f';outname[i+4]=0;
    }


    static const char ELF_STUB[] =
        "\x7f""ELF\x01\x01\x01\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x02\x00"
        "\x03\x00"
        "\x01\x00\x00\x00"
        "[stinger-stub]";

    sfs_node_t *parent=sfs_parent(cwd,outname);
    if(!parent) parent=cwd;

    const char *bname=outname;
    for(int i=0;outname[i];i++) if(outname[i]=='/'||outname[i]=='\\') bname=outname+i+1;
    sfs_write_file(parent,bname,(const uint8_t*)ELF_STUB,sizeof(ELF_STUB)-1);

    terminal_printf("gcc: output written to '%s'\n",outname);
    terminal_printf("gcc: (stinger stub — use host cross-compiler for real exec)\n");
    if(verbose){
        terminal_printf("     source: %d lines, %d bytes\n",lines,(int)slen);
    }
}




