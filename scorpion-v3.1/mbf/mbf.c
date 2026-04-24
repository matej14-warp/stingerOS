/* scorpion OS - mbf/mbf.c
   Manly Batch File interpreter
   Syntax:
     echo <text>
     set <var> <value>
     if <var> == <value> { ... } else { ... }
     install <path>
     copy <src> <dst>
     mkdir <path>
     rm <path>
     reboot
     halt
     # comment                                          */

#include "mbf.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include "../fs/sfs.h"
#include <stdint.h>
#include <stddef.h>

/* ---- helpers ---- */
static size_t slen(const char*s){size_t n=0;while(s[n])n++;return n;}
static int scmp(const char*a,const char*b){
    while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b;
}
static void scpy(char*d,const char*s){while((*d++=*s++));}
static int sisspace(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
static int sisalnum(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';}

static char *trim(char *s){
    while(*s&&sisspace(*s))s++;
    char *e=s+(int)slen(s)-1;
    while(e>s&&sisspace(*e)) *e--=0;
    return s;
}

/* Split line into tokens (space-separated), returns count */
static int tokenize(char *line, char **toks, int max) {
    int n=0;
    char *p=line;
    while(*p&&n<max){
        while(*p&&sisspace(*p))p++;
        if(!*p) break;
        toks[n++]=p;
        while(*p&&!sisspace(*p))p++;
        if(*p) *p++=0;
    }
    return n;
}

/* ---- Variable store ---- */
#define MBF_MAX_VARS 32
typedef struct { char name[32]; char value[128]; } mbf_var_t;
static mbf_var_t vars[MBF_MAX_VARS];
static int var_count=0;

static void var_set(const char *name, const char *val){
    for(int i=0;i<var_count;i++){
        if(scmp(vars[i].name,name)==0){
            scpy(vars[i].value,val);return;
        }
    }
    if(var_count<MBF_MAX_VARS){
        scpy(vars[var_count].name,name);
        scpy(vars[var_count].value,val);
        var_count++;
    }
}

static const char *var_get(const char *name){
    for(int i=0;i<var_count;i++)
        if(scmp(vars[i].name,name)==0) return vars[i].value;
    return "";
}

/* Expand $VAR in string into out_buf */
static void expand(const char *in, char *out, size_t outsz){
    size_t o=0;
    while(*in&&o<outsz-1){
        if(*in=='$'){
            in++;
            char vname[32]; int vi=0;
            while(*in&&sisalnum(*in)&&vi<31) vname[vi++]=(char)*in++;
            vname[vi]=0;
            const char *val=var_get(vname);
            while(*val&&o<outsz-1) out[o++]=*val++;
        } else {
            out[o++]=*in++;
        }
    }
    out[o]=0;
}

/* ---- extract last path component ---- */
static const char *basename_of(const char *path) {
    const char *last = path;
    for (const char *p = path; *p; p++)
        if (*p == '/') last = p + 1;
    return last;
}

/* ---- Line-based execution ---- */
static int mbf_exec_line(const char *raw_line, const char **all_lines,
                          int all_count, int *ip, sfs_node_t *cwd) {
    char line[512]; int li=0;
    while(*raw_line&&li<511) line[li++]=(char)*raw_line++;
    line[li]=0;
    char *p=trim(line);

    if(!*p||p[0]=='#') return 0; /* empty or comment */

    char *toks[16]; int ntok;
    char expanded[512];
    expand(p, expanded, sizeof(expanded));
    p=expanded;
    ntok=tokenize(p,toks,16);
    if(ntok==0) return 0;

    /* echo */
    if(scmp(toks[0],"echo")==0){
        for(int i=1;i<ntok;i++){
            terminal_writestring(toks[i]);
            if(i<ntok-1) terminal_putchar(' ');
        }
        terminal_putchar('\n');
        return 0;
    }

    /* set <var> <value> */
    if(scmp(toks[0],"set")==0&&ntok>=3){
        var_set(toks[1], toks[2]);
        return 0;
    }

    /* mkdir <path> */
    if(scmp(toks[0],"mkdir")==0&&ntok>=2){
        sfs_node_t *dir=sfs_resolve(cwd,toks[1]);
        if(!dir){
            /* Walk path, creating each component */
            char pbuf[256];
            scpy(pbuf, toks[1]);
            /* Find last slash to split parent / leaf */
            char *last_slash = NULL;
            for(char *q=pbuf; *q; q++)
                if(*q=='/') last_slash=q;

            if(last_slash && last_slash > pbuf){
                *last_slash = 0;
                const char *leaf = last_slash + 1;
                sfs_node_t *parent = sfs_resolve(cwd, pbuf);
                if(!parent) parent = cwd;
                if(*leaf) sfs_mkdir(parent, leaf);
            } else {
                /* No slash - create directly under cwd */
                const char *leaf = (last_slash) ? last_slash+1 : toks[1];
                if(*leaf) sfs_mkdir(cwd, leaf);
            }
        }
        return 0;
    }

    /* copy <src> <dst> */
    if(scmp(toks[0],"copy")==0&&ntok>=3){
        sfs_node_t *src=sfs_resolve(cwd,toks[1]);
        if(!src||src->type!=SFS_FILE){
            terminal_printf("[mbf] copy: source not found: %s\n",toks[1]);
            return 1;
        }
        sfs_node_t *dst_parent=sfs_resolve(cwd,toks[2]);
        if(!dst_parent) dst_parent=cwd;
        /* extract filename from src */
        const char *fname=src->name;
        sfs_write_file(dst_parent,fname,src->data,src->size);
        return 0;
    }

    /* rm <path> */
    if(scmp(toks[0],"rm")==0&&ntok>=2){
        sfs_node_t *n=sfs_resolve(cwd,toks[1]);
        if(n) sfs_delete(n);
        else terminal_printf("[mbf] rm: not found: %s\n",toks[1]);
        return 0;
    }

    /* install <path> - move file to /bin */
    if(scmp(toks[0],"install")==0&&ntok>=2){
        sfs_node_t *src=sfs_resolve(cwd,toks[1]);
        if(!src||src->type!=SFS_FILE){
            terminal_printf("[mbf] install: not found: %s\n",toks[1]);
            return 1;
        }
        sfs_node_t *bin=sfs_resolve(sfs_root(),"/bin");
        if(!bin){terminal_printf("[mbf] install: /bin not found\n");return 1;}
        sfs_write_file(bin,src->name,src->data,src->size);
        terminal_printf("[mbf] installed: %s -> /bin/%s\n",toks[1],src->name);
        return 0;
    }

    /* reboot */
    if(scmp(toks[0],"reboot")==0){
        terminal_printf("[mbf] rebooting...\n");
        uint8_t tmp;
        do { __asm__ volatile("inb $0x64,%0":"=a"(tmp)); } while(tmp&2);
        __asm__ volatile("outb %0,$0x64"::"a"((uint8_t)0xFE));
        __asm__ volatile("hlt");
        return 0;
    }

    /* halt */
    if(scmp(toks[0],"halt")==0){
        terminal_printf("[mbf] system halted\n");
        __asm__ volatile("cli; hlt");
        return 0;
    }

    /* if <var> == <value> { ... } [else { ... }] */
    if(scmp(toks[0],"if")==0){
        if(ntok<4){
            terminal_printf("[mbf] if: syntax error\n");
            return 1;
        }
        const char *lhs=var_get(toks[1]);
        int cond=0;
        if(scmp(toks[2],"==")==0)      cond=(scmp(lhs,toks[3])==0);
        else if(scmp(toks[2],"!=")==0) cond=(scmp(lhs,toks[3])!=0);
        else { terminal_printf("[mbf] if: unknown operator %s\n",toks[2]); return 1; }

        /* Collect then-block lines until matching } */
        int then_start=*ip+1, then_end=-1;
        int else_start=-1, else_end=-1;
        int depth=0;
        for(int i=*ip;i<all_count;i++){
            char tmp2[256]; int ti=0;
            const char *ll=all_lines[i];
            while(*ll&&ti<255) tmp2[ti++]=(char)*ll++;
            tmp2[ti]=0;
            char *tt=trim(tmp2);
            for(char*q=tt;*q;q++){
                if(*q=='{') depth++;
                else if(*q=='}'){
                    depth--;
                    if(depth==0){
                        then_end=i;
                        /* check for else on next line */
                        if(i+1<all_count){
                            char tmp3[256]; int ti3=0;
                            const char *ll2=all_lines[i+1];
                            while(*ll2&&ti3<255) tmp3[ti3++]=(char)*ll2++;
                            tmp3[ti3]=0;
                            char *te=trim(tmp3);
                            if(te[0]=='e'&&te[1]=='l'&&te[2]=='s'&&te[3]=='e'){
                                else_start=i+2;
                                /* find closing } of else block */
                                int d2=0;
                                for(int j=i+1;j<all_count;j++){
                                    char tmp4[256]; int ti4=0;
                                    const char *ll3=all_lines[j];
                                    while(*ll3&&ti4<255) tmp4[ti4++]=(char)*ll3++;
                                    tmp4[ti4]=0;
                                    char *te2=trim(tmp4);
                                    for(char *q2=te2;*q2;q2++){
                                        if(*q2=='{') d2++;
                                        else if(*q2=='}'){
                                            d2--;
                                            if(d2==0){else_end=j;break;}
                                        }
                                    }
                                    if(else_end>=0) break;
                                }
                                *ip=(else_end>=0)?else_end:then_end;
                            } else {
                                *ip=then_end;
                            }
                        } else {
                            *ip=then_end;
                        }
                        break;
                    }
                }
            }
            if(then_end>=0) break;
        }

        if(cond){
            int sub_ip=then_start;
            while(sub_ip<(then_end>=0?then_end:all_count)){
                mbf_exec_line(all_lines[sub_ip],all_lines,all_count,&sub_ip,cwd);
                sub_ip++;
            }
        } else if(else_start>=0&&else_end>=0){
            int sub_ip=else_start;
            while(sub_ip<else_end){
                mbf_exec_line(all_lines[sub_ip],all_lines,all_count,&sub_ip,cwd);
                sub_ip++;
            }
        }
        return 0;
    }

    /* Unknown command */
    terminal_printf("[mbf] unknown command: %s\n",toks[0]);
    return 0;
}

/* suppress unused warning for basename_of if not used elsewhere */
static const char *(*_bof_ref)(const char*) = basename_of;

/* ---- Public API ---- */

int mbf_run(const char *script_text, size_t script_len, sfs_node_t *cwd) {
    (void)_bof_ref;
    if(!script_text||script_len==0) return -1;

    char *buf=(char*)kmalloc(script_len+1);
    if(!buf) return -1;
    for(size_t i=0;i<script_len;i++) buf[i]=script_text[i];
    buf[script_len]=0;

    const char *lines[512];
    int nlines=0;
    char *p=buf;
    lines[nlines++]=p;
    while(*p&&nlines<512){
        if(*p=='\n'){ *p=0; lines[nlines++]=p+1; }
        else if(*p=='\r') *p=0;
        p++;
    }

    var_count=0;
    int ip=0;
    while(ip<nlines){
        mbf_exec_line(lines[ip],lines,nlines,&ip,cwd);
        ip++;
    }

    kfree(buf);
    return 0;
}

int mbf_run_file(const char *path, sfs_node_t *cwd) {
    sfs_node_t *n=sfs_resolve(cwd,path);
    if(!n||n->type!=SFS_FILE){
        terminal_printf("[mbf] file not found: %s\n",path);
        return -1;
    }
    return mbf_run((const char*)n->data,n->size,cwd);
}
