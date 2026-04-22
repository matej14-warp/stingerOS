











static void *py_alloc(size_t n){ return kmalloc(n); }
static void  py_free(void *p)  { kfree(p); }

static int py_streq(const char *a, const char *b){
    while(*a && *b && *a == *b){ a++; b++; }
    return (*a == 0 && *b == 0);
}
static int py_slen(const char *s){ int n=0; while(s[n]) n++; return n; }
static void py_scpy(char *d, const char *s, int max){
    int i=0; while(s[i] && i<max-1){ d[i]=s[i]; i++; } d[i]=0;
}
static void py_print_str(const char *s){ terminal_writestring(s); }
static void py_print_int(int v){
    if(v < 0){ terminal_putchar('-'); v = -v; }
    char buf[12]; int i=0;
    if(v==0){ terminal_putchar('0'); return; }
    while(v){ buf[i++]='0'+v%10; v/=10; }
    while(i>0) terminal_putchar(buf[--i]);
}
static void py_print_float(float f){
    if(f < 0){ terminal_putchar('-'); f=-f; }
    int integer = (int)f;
    py_print_int(integer);
    terminal_putchar('.');
    int frac = (int)((f - (float)integer) * 1000.0f);
    if(frac < 0) frac = -frac;
    char fb[4]; int fi=0;
    if(!frac){ terminal_writestring("000"); return; }
    while(fi<3){ fb[fi++]='0'+frac%10; frac/=10; }
    while(fi>0) terminal_putchar(fb[--fi]);
}
static void py_itoa(char *buf, int v){
    if(v==0){ buf[0]='0'; buf[1]=0; return; }
    int neg=0; if(v<0){neg=1;v=-v;}
    char tmp[12]; int i=0;
    while(v){ tmp[i++]='0'+v%10; v/=10; }
    if(neg) tmp[i++]='-';
    int j=0; while(i>0) buf[j++]=tmp[--i]; buf[j]=0;
}
static int py_atoi(const char *s){
    int neg=0,v=0; if(*s=='-'){neg=1;s++;}
    while(*s>='0'&&*s<='9'){ v=v*10+(*s-'0'); s++; }
    return neg?-v:v;
}












































typedef struct {
    int   type;
    char  val[MAX_TOKLEN];
    int   ival;
    float fval;
    int   indent;
    int   line;
} py_tok_t;

typedef struct {
    const char *src;
    int         pos;
    int         line;
    py_tok_t   *toks;
    int         ntoks;
    int         cap;
} py_lex_t;

static void lex_emit(py_lex_t *lx, py_tok_t *t){
    if(lx->ntoks >= lx->cap) return;
    lx->toks[lx->ntoks++] = *t;
}

static int is_alpha(char c){ return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'; }
static int is_digit(char c){ return c>='0'&&c<='9'; }
static int is_alnum(char c){ return is_alpha(c)||is_digit(c); }
static int is_ws(char c)   { return c==' '||c=='\t'; }

static void lex_run(py_lex_t *lx){
    int indent_stack[64]; int istk_top=0;
    indent_stack[0]=0;
    int pos = lx->pos;
    const char *s = lx->src;
    int line = 1;

    while(s[pos]){

        if(s[pos]=='\n'){ pos++; line++; continue; }
        if(s[pos]=='
            while(s[pos]&&s[pos]!='\n') pos++;
            continue;
        }

        int col=0; int lstart=pos;
        while(s[pos]==' '){ col++; pos++; }
        while(s[pos]=='\t'){ col+=4; pos++; }
        if(s[pos]=='\n'||s[pos]==0||s[pos]=='


        int cur_indent = indent_stack[istk_top];
        if(col > cur_indent){
            if(istk_top < 63) indent_stack[++istk_top] = col;
            py_tok_t it={0}; it.type=TOK_INDENT; it.indent=col; it.line=line;
            lex_emit(lx, &it);
        } else {
            while(istk_top > 0 && indent_stack[istk_top] > col){
                py_tok_t dt={0}; dt.type=TOK_DEDENT; dt.indent=col; dt.line=line;
                lex_emit(lx, &dt);
                istk_top--;
            }
        }


        while(s[pos] && s[pos]!='\n'){
            char c = s[pos];
            if(c=='
            if(is_ws(c)){ pos++; continue; }

            py_tok_t t={0}; t.line=line;


            if(c=='"'||c=='\''){
                char delim=c; pos++;
                int si=0;
                while(s[pos]&&s[pos]!=delim&&si<MAX_TOKLEN-1){
                    if(s[pos]=='\\'){
                        pos++;
                        char esc=s[pos++];
                        if(esc=='n')      t.val[si++]='\n';
                        else if(esc=='t') t.val[si++]='\t';
                        else if(esc=='\\')t.val[si++]='\\';
                        else if(esc=='\'')t.val[si++]='\'';
                        else if(esc=='"') t.val[si++]='"';
                        else { t.val[si++]='\\'; t.val[si++]=esc; }
                    } else {
                        t.val[si++]=s[pos++];
                    }
                }
                t.val[si]=0;
                if(s[pos]==delim) pos++;
                t.type=TOK_STR;
                lex_emit(lx,&t);
                continue;
            }


            if(is_digit(c)||(c=='-'&&is_digit(s[pos+1]))){
                int neg=(c=='-'); if(neg) pos++;
                int vi=0; int is_float=0; float fv=0;
                while(is_digit(s[pos])) vi=vi*10+(s[pos++]-'0');
                if(s[pos]=='.'){
                    is_float=1; pos++;
                    float dec=0.1f;
                    while(is_digit(s[pos])){fv+=(s[pos++]-'0')*dec;dec*=0.1f;}
                    fv+=(float)vi;
                    if(neg) fv=-fv;
                }
                if(is_float){
                    t.type=TOK_FLOAT; t.fval=fv;
                    py_itoa(t.val,(int)fv);
                } else {
                    t.type=TOK_INT; t.ival=neg?-vi:vi;
                    py_itoa(t.val,t.ival);
                }
                lex_emit(lx,&t);
                continue;
            }


            if(is_alpha(c)){
                int ni=0;
                while(is_alnum(s[pos])&&ni<MAX_TOKLEN-1) t.val[ni++]=s[pos++];
                t.val[ni]=0;

                if(py_streq(t.val,"and"))    t.type=TOK_AND;
                else if(py_streq(t.val,"or"))  t.type=TOK_OR;
                else if(py_streq(t.val,"not")) t.type=TOK_NOT;
                else if(py_streq(t.val,"pass"))t.type=TOK_PASS;
                else                           t.type=TOK_NAME;
                lex_emit(lx,&t);
                continue;
            }


            pos++;
            switch(c){
            case '=':
                if(s[pos]=='='){ pos++; t.type=TOK_EQ; t.val[0]='=';t.val[1]='=';t.val[2]=0; }
                else { t.type=TOK_ASSIGN; t.val[0]='=';t.val[1]=0; }
                break;
            case '!':
                if(s[pos]=='='){ pos++; t.type=TOK_NE; t.val[0]='!';t.val[1]='=';t.val[2]=0; }
                break;
            case '<':
                if(s[pos]=='='){ pos++; t.type=TOK_LE; t.val[0]='<';t.val[1]='=';t.val[2]=0; }
                else { t.type=TOK_LT; t.val[0]='<';t.val[1]=0; }
                break;
            case '>':
                if(s[pos]=='='){ pos++; t.type=TOK_GE; t.val[0]='>';t.val[1]='=';t.val[2]=0; }
                else { t.type=TOK_GT; t.val[0]='>';t.val[1]=0; }
                break;
            case '+':
                if(s[pos]=='='){ pos++; t.type=TOK_PLUSEQ; }
                else t.type=TOK_PLUS; t.val[0]=c; t.val[1]=0; break;
            case '-':
                if(s[pos]=='='){ pos++; t.type=TOK_MINUSEQ; }
                else t.type=TOK_MINUS; t.val[0]=c; t.val[1]=0; break;
            case '*':
                if(s[pos]=='='){ pos++; t.type=TOK_STAREQ; }
                else t.type=TOK_STAR; t.val[0]=c; t.val[1]=0; break;
            case '/':
                if(s[pos]=='='){ pos++; t.type=TOK_SLASHEQ; }
                else t.type=TOK_SLASH; t.val[0]=c; t.val[1]=0; break;
            case '%': t.type=TOK_PERCENT; t.val[0]=c; t.val[1]=0; break;
            case ':': t.type=TOK_COLON;   t.val[0]=c; t.val[1]=0; break;
            case ',': t.type=TOK_COMMA;   t.val[0]=c; t.val[1]=0; break;
            case '(': t.type=TOK_LPAREN;  t.val[0]=c; t.val[1]=0; break;
            case ')': t.type=TOK_RPAREN;  t.val[0]=c; t.val[1]=0; break;
            case '[': t.type=TOK_LBRAK;   t.val[0]=c; t.val[1]=0; break;
            case ']': t.type=TOK_RBRAK;   t.val[0]=c; t.val[1]=0; break;
            case '.': t.type=TOK_DOT;     t.val[0]=c; t.val[1]=0; break;
            default:  continue;
            }
            lex_emit(lx,&t);
        }


        py_tok_t nl={0}; nl.type=TOK_NEWLINE; nl.line=line;
        lex_emit(lx, &nl);
        if(s[pos]=='\n'){ pos++; line++; }
    }


    py_tok_t eof={0}; eof.type=TOK_EOF;
    lex_emit(lx,&eof);
}















typedef struct py_val_t py_val_t;
struct py_val_t {
    int type;
    union {
        int ival;
        float fval;
        struct {
            char *sval;
            int slen;
        } str;
        struct {
            py_val_t **list;
            int list_len;
            int list_cap;
        } list;
        struct {
            int func_tok_start;
            const char *param_names[8];
            int param_count;
        } func;
    } data;
};

static py_val_t PY_NONE_VAL = {PY_NIL, {.ival=0}};
static py_val_t PY_TRUE_VAL  = {PY_BOOL, {.ival=1}};
static py_val_t PY_FALSE_VAL = {PY_BOOL, {.ival=0}};

static py_val_t *py_int_val(int v){
    py_val_t *x = (py_val_t*)py_alloc(sizeof(py_val_t));
    if(!x) return &PY_NONE_VAL;
    x->type=PY_INT; x->data.ival=v;
    return x;
}
static py_val_t *py_float_val(float f){
    py_val_t *x = (py_val_t*)py_alloc(sizeof(py_val_t));
    if(!x) return &PY_NONE_VAL;
    x->type=PY_FLOAT; x->data.fval=f;
    return x;
}
static py_val_t *py_str_val(const char *s){
    py_val_t *x = (py_val_t*)py_alloc(sizeof(py_val_t));
    if(!x) return &PY_NONE_VAL;
    x->type=PY_STR;
    int slen = py_slen(s);
    x->data.str.sval = (char*)py_alloc(slen+1);
    if(!x->data.str.sval) return &PY_NONE_VAL;
    py_scpy(x->data.str.sval, s, slen+1);
    x->data.str.slen = slen;
    return x;
}
static py_val_t *py_bool_val(int b){
    return b ? &PY_TRUE_VAL : &PY_FALSE_VAL;
}
static py_val_t *py_list_val(void){
    py_val_t *x = (py_val_t*)py_alloc(sizeof(py_val_t));
    if(!x) return &PY_NONE_VAL;
    x->type=PY_LIST;
    x->data.list.list = (py_val_t**)py_alloc(sizeof(py_val_t*)*PY_LIST_MAX);
    if(!x->data.list.list) return &PY_NONE_VAL;
    x->data.list.list_len = 0;
    x->data.list.list_cap = PY_LIST_MAX;
    return x;
}

static int py_truthy(py_val_t *v){
    if(!v) return 0;
    switch(v->type){
    case PY_BOOL: return v->data.ival;
    case PY_INT:  return v->data.ival != 0;
    case PY_FLOAT:return v->data.fval != 0.0f;
    case PY_STR:  return v->data.str.sval && v->data.str.sval[0] != 0;
    case PY_LIST: return v->data.list.list_len > 0;
    case PY_NIL:  return 0;
    default:      return 1;
    }
}

static void py_val_to_str(py_val_t *v, char *buf, int max){
    if(!v){ py_scpy(buf,"None",max); return; }
    switch(v->type){
    case PY_NIL:   py_scpy(buf,"None",max);  break;
    case PY_BOOL:  py_scpy(buf,v->data.ival?"True":"False",max); break;
    case PY_INT:   py_itoa(buf,v->data.ival); break;
    case PY_FLOAT: { py_itoa(buf,(int)v->data.fval); int l=py_slen(buf); if(l<max-4){buf[l]='.';py_itoa(buf+l+1,(int)((v->data.fval-(float)(int)v->data.fval)*1000.0f));} break; }
    case PY_STR:   py_scpy(buf,v->data.str.sval ? v->data.str.sval : "",max); break;
    case PY_LIST:  { int p=0; buf[p++]='[';
        for(int i=0;i<v->data.list.list_len&&p<max-4;i++){
            if(i>0){buf[p++]=',';buf[p++]=' ';}
            char tmp[64]; py_val_to_str(v->data.list.list[i],tmp,64);
            int tl=py_slen(tmp); if(p+tl<max-2){ for(int j=0;j<tl;j++)buf[p++]=tmp[j]; }
        }
        buf[p++]=']'; buf[p]=0; break; }
    case PY_WIN:   py_scpy(buf,"<Window>",max); break;
    case PY_FUNC:  py_scpy(buf,"<function>",max); break;
    default:       py_scpy(buf,"<object>",max); break;
    }
}






typedef struct {
    char      name[PY_VAR_NAMELEN];
    py_val_t *val;
} py_var_t;

typedef struct py_env_t py_env_t;
struct py_env_t {
    py_var_t   vars[PY_MAX_VARS];
    int        nv;
    py_env_t  *parent;
};

static py_val_t *py_env_get(py_env_t *env, const char *name){
    for(py_env_t *e=env;e;e=e->parent){
        for(int i=0;i<e->nv;i++){
            if(py_streq(e->vars[i].name,name))
                return e->vars[i].val;
        }
    }
    return NULL;
}

static void py_env_set(py_env_t *env, const char *name, py_val_t *val){

    for(int i=0;i<env->nv;i++){
        if(py_streq(env->vars[i].name,name)){
            env->vars[i].val=val; return;
        }
    }

    if(env->nv < PY_MAX_VARS){
        py_scpy(env->vars[env->nv].name, name, PY_VAR_NAMELEN);
        env->vars[env->nv].val = val;
        env->nv++;
    }
}

static py_env_t *py_env_push(py_env_t *parent){
    py_env_t *e = (py_env_t*)py_alloc(sizeof(py_env_t));
    if(!e) return parent;
    e->nv=0; e->parent=parent;
    return e;
}
static void py_env_pop(py_env_t *e){ py_free(e); }





typedef struct {
    char name[PIP_NAMELEN];
    char version[16];
    int  installed;
} pip_pkg_t;

static pip_pkg_t g_pip_db[PIP_MAX_PKGS];
static int       g_pip_count = 0;

static int pip_find(const char *name){
    for(int i=0;i<g_pip_count;i++)
        if(py_streq(g_pip_db[i].name,name)) return i;
    return -1;
}

void python_pip_install(const char *pkg){
    int idx = pip_find(pkg);
    if(idx >= 0 && g_pip_db[idx].installed){
        terminal_printf("Requirement already satisfied: %s\n", pkg);
        return;
    }
    if(idx < 0){
        if(g_pip_count >= PIP_MAX_PKGS){
            terminal_printf("pip: package database full\n");
            return;
        }
        idx = g_pip_count++;
        py_scpy(g_pip_db[idx].name, pkg, PIP_NAMELEN);
        py_scpy(g_pip_db[idx].version, "1.0.0", 16);
    }
    g_pip_db[idx].installed = 1;
    terminal_printf("Collecting %s\n", pkg);
    terminal_printf("  Downloading %s-1.0.0-py3-none-any.whl (simulated)\n", pkg);
    terminal_printf("Installing collected packages: %s\n", pkg);
    terminal_printf("Successfully installed %s-1.0.0\n", pkg);
}

void python_pip_remove(const char *pkg){
    int idx = pip_find(pkg);
    if(idx < 0 || !g_pip_db[idx].installed){
        terminal_printf("WARNING: Skipping %s as it is not installed.\n", pkg);
        return;
    }
    g_pip_db[idx].installed = 0;
    terminal_printf("Successfully uninstalled %s-1.0.0\n", pkg);
}

void python_pip_list(void){
    terminal_printf("Package              Version\n");
    terminal_printf("-------------------- -------\n");
    int found = 0;
    for(int i=0;i<g_pip_count;i++){
        if(g_pip_db[i].installed){
            terminal_printf("%-20s %s\n", g_pip_db[i].name, g_pip_db[i].version);
            found=1;
        }
    }

    terminal_printf("%-20s %s\n","stinger","built-in");
    terminal_printf("%-20s %s\n","math","built-in");
    terminal_printf("%-20s %s\n","sys","built-in");
    terminal_printf("%-20s %s\n","os","built-in");
    terminal_printf("%-20s %s\n","random","built-in");
    terminal_printf("%-20s %s\n","time","built-in");
    (void)found;
}




typedef struct py_interp_t py_interp_t;
static py_val_t *py_eval_expr(py_interp_t *interp, py_env_t *env, int *tp);
static int       py_exec_stmts(py_interp_t *interp, py_env_t *env, int *tp, int dedent_stop);







struct py_interp_t {
    py_tok_t  *toks;
    int        ntoks;
    py_val_t  *return_val;
    int        status;
    char       errmsg[128];
};

static py_tok_t *cur_tok(py_interp_t *interp, int tp){
    if(tp >= interp->ntoks) return &interp->toks[interp->ntoks-1];
    return &interp->toks[tp];
}

static int peek_type(py_interp_t *interp, int tp){
    return cur_tok(interp,tp)->type;
}

static py_tok_t *consume(py_interp_t *interp, int *tp){
    py_tok_t *t = cur_tok(interp, *tp);
    (*tp)++;
    return t;
}

static py_tok_t *expect(py_interp_t *interp, int *tp, int type){
    py_tok_t *t = consume(interp, tp);
    if(t->type != type){
        interp->status = PY_RET_ERROR;
        terminal_printf("SyntaxError: expected token %d got %d (line %d)\n", type, t->type, t->line);
    }
    return t;
}


static void skip_to_newline(py_interp_t *interp, int *tp){
    while(peek_type(interp,*tp) != TOK_NEWLINE &&
          peek_type(interp,*tp) != TOK_EOF)
        consume(interp,tp);
    if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
}


static void skip_block(py_interp_t *interp, int *tp){

    if(peek_type(interp,*tp)==TOK_INDENT) consume(interp,tp);
    int depth=1;
    while(depth > 0 && peek_type(interp,*tp) != TOK_EOF){
        int t=peek_type(interp,*tp);
        consume(interp,tp);
        if(t==TOK_INDENT) depth++;
        if(t==TOK_DEDENT){ depth--; }
    }
}


static py_val_t *py_call_builtin(py_interp_t *interp, py_env_t *env,
                                  const char *name, py_val_t **args, int nargs)
{
    (void)interp;

    if(py_streq(name,"print")){
        for(int i=0;i<nargs;i++){
            if(i>0) terminal_putchar(' ');
            char buf[PY_STR_MAX]; py_val_to_str(args[i],buf,PY_STR_MAX);
            terminal_writestring(buf);
        }
        terminal_putchar('\n');
        return &PY_NONE_VAL;
    }

    if(py_streq(name,"input")){
        if(nargs>0){ char pb[PY_STR_MAX]; py_val_to_str(args[0],pb,PY_STR_MAX); terminal_writestring(pb); }
        char buf[256]; int bi=0;
        char c;
        while((c=keyboard_getchar())!='\n'&&c!='\r'&&bi<255){
            terminal_putchar(c); buf[bi++]=c;
        }
        buf[bi]=0; terminal_putchar('\n');
        return py_str_val(buf);
    }

    if(py_streq(name,"len")){
        if(nargs<1) return py_int_val(0);
        if(args[0]->type==PY_STR) return py_int_val(args[0]->data.str.slen);
        if(args[0]->type==PY_LIST) return py_int_val(args[0]->data.list.list_len);
        return py_int_val(0);
    }

    if(py_streq(name,"int")){
        if(nargs<1) return py_int_val(0);
        if(args[0]->type==PY_INT) return args[0];
        if(args[0]->type==PY_FLOAT) return py_int_val((int)args[0]->data.fval);
        if(args[0]->type==PY_STR) return py_int_val(py_atoi(args[0]->data.str.sval));
        return py_int_val(0);
    }

    if(py_streq(name,"str")){
        if(nargs<1) return py_str_val("");
        char buf[PY_STR_MAX]; py_val_to_str(args[0],buf,PY_STR_MAX);
        return py_str_val(buf);
    }

    if(py_streq(name,"float")){
        if(nargs<1) return py_float_val(0.0f);
        if(args[0]->type==PY_FLOAT) return args[0];
        if(args[0]->type==PY_INT) return py_float_val((float)args[0]->data.ival);
        return py_float_val(0.0f);
    }

    if(py_streq(name,"range")){
        int start=0,stop=0,step=1;
        if(nargs==1){ stop=args[0]->data.ival; }
        else if(nargs>=2){ start=args[0]->data.ival; stop=args[1]->data.ival; }
        if(nargs>=3) step=args[2]->data.ival;
        py_val_t *lst = py_list_val();
        for(int i=start; (step>0?(i<stop):(i>stop)) && lst->data.list.list_len<PY_LIST_MAX; i+=step){

            if(!lst->data.list.list){
                lst->data.list.list = (py_val_t**)py_alloc(sizeof(py_val_t*)*PY_LIST_MAX);
                if(!lst->data.list.list) break;
                lst->data.list.list_cap = PY_LIST_MAX;
            }
            lst->data.list.list[lst->data.list.list_len++] = py_int_val(i);
        }
        return lst;
    }

    if(py_streq(name,"list")){
        if(nargs<1) return py_list_val();
        if(args[0]->type==PY_LIST) return args[0];
        return py_list_val();
    }

    if(py_streq(name,"abs")){
        if(nargs<1) return py_int_val(0);
        if(args[0]->type==PY_FLOAT) return py_float_val(args[0]->data.fval<0?-args[0]->data.fval:args[0]->data.fval);
        return py_int_val(args[0]->data.ival<0?-args[0]->data.ival:args[0]->data.ival);
    }

    if(py_streq(name,"max")||py_streq(name,"min")){
        if(nargs<1) return &PY_NONE_VAL;
        int best=args[0]->data.ival; float bestf=args[0]->data.fval;
        for(int i=1;i<nargs;i++){
            int cur=args[i]->data.ival; float curf=args[i]->data.fval;
            if(py_streq(name,"max")){ if(cur>best){best=cur;bestf=curf;} }
            else { if(cur<best){best=cur;bestf=curf;} }
        }
        if(nargs>0&&args[0]->type==PY_FLOAT) return py_float_val(bestf);
        return py_int_val(best);
    }

    if(py_streq(name,"exit")||py_streq(name,"quit")){
        interp->status=PY_RET_RETURN;
        interp->return_val=&PY_NONE_VAL;
        return &PY_NONE_VAL;
    }

    if(py_streq(name,"type")){
        if(nargs<1) return py_str_val("<class 'NoneType'>");
        switch(args[0]->type){
        case PY_INT:  return py_str_val("<class 'int'>");
        case PY_FLOAT:return py_str_val("<class 'float'>");
        case PY_STR:  return py_str_val("<class 'str'>");
        case PY_LIST: return py_str_val("<class 'list'>");
        case PY_BOOL: return py_str_val("<class 'bool'>");
        case PY_WIN:  return py_str_val("<class 'stinger.Window'>");
        default:      return py_str_val("<class 'object'>");
        }
    }


    if(py_streq(name,"stinger.sleep")||py_streq(name,"time.sleep")){
        if(nargs>0){
            int ms = args[0]->type==PY_FLOAT ? (int)(args[0]->data.fval*1000) : args[0]->data.ival*1000;
            for(int m=0;m<ms;m++) for(volatile int i=0;i<4000;i++);
        }
        return &PY_NONE_VAL;
    }
    if(py_streq(name,"stinger.print_ok")){
        terminal_printf("[stinger] OK\n");
        return &PY_NONE_VAL;
    }


    py_val_t *fn = py_env_get(env, name);
    if(fn && fn->type == PY_FUNC){

        py_env_t *fenv = py_env_push(env);
        for(int i=0;i<fn->data.func.param_count&&i<nargs;i++){
            py_env_set(fenv, fn->data.func.param_names[i], args[i]);
        }
        int ftp = fn->data.func.func_tok_start;
        py_exec_stmts(interp, fenv, &ftp, 1);
        py_val_t *rv = interp->return_val ? interp->return_val : &PY_NONE_VAL;
        if(interp->status == PY_RET_RETURN) interp->status = PY_RET_NORMAL;
        interp->return_val = NULL;
        py_env_pop(fenv);
        return rv;
    }

    terminal_printf("NameError: name '%s' is not defined\n", name);
    interp->status = PY_RET_ERROR;
    return &PY_NONE_VAL;
}


static py_val_t *py_call_method(py_interp_t *interp, py_val_t *obj,
                                 const char *method, py_val_t **args, int nargs)
{
    (void)interp;

    if(obj->type == PY_LIST){
        if(py_streq(method,"append")){
            if(nargs>0 && obj->data.list.list_len < PY_LIST_MAX){

                if(!obj->data.list.list){
                    obj->data.list.list = (py_val_t**)py_alloc(sizeof(py_val_t*)*PY_LIST_MAX);
                    if(!obj->data.list.list) return &PY_NONE_VAL;
                    obj->data.list.list_cap = PY_LIST_MAX;
                }
                obj->data.list.list[obj->data.list.list_len++] = args[0];
            }
            return &PY_NONE_VAL;
        }
        if(py_streq(method,"pop")){
            if(obj->data.list.list && obj->data.list.list_len>0){
                py_val_t *v=obj->data.list.list[--obj->data.list.list_len];
                return v;
            }
            return &PY_NONE_VAL;
        }
        if(py_streq(method,"clear")){
            if(obj->data.list.list) obj->data.list.list_len=0;
            return &PY_NONE_VAL;
        }
    }

    if(obj->type == PY_STR){
        if(py_streq(method,"upper")){
            char buf[PY_STR_MAX]; int i=0;
            const char *src = obj->data.str.sval ? obj->data.str.sval : "";
            while(src[i]&&i<PY_STR_MAX-1){
                char c=src[i]; if(c>='a'&&c<='z') c-=32; buf[i++]=c;
            } buf[i]=0; return py_str_val(buf);
        }
        if(py_streq(method,"lower")){
            char buf[PY_STR_MAX]; int i=0;
            const char *src = obj->data.str.sval ? obj->data.str.sval : "";
            while(src[i]&&i<PY_STR_MAX-1){
                char c=src[i]; if(c>='A'&&c<='Z') c+=32; buf[i++]=c;
            } buf[i]=0; return py_str_val(buf);
        }
        if(py_streq(method,"strip")){
            const char *p=obj->data.str.sval ? obj->data.str.sval : ""; while(*p==' '||*p=='\t')p++;
            int l=py_slen(p); while(l>0&&(p[l-1]==' '||p[l-1]=='\t'))l--;
            char buf[PY_STR_MAX]; int i=0; while(i<l&&i<PY_STR_MAX-1){buf[i]=p[i];i++;} buf[i]=0;
            return py_str_val(buf);
        }
        if(py_streq(method,"split")){
            py_val_t *lst=py_list_val();
            const char *p=obj->data.str.sval ? obj->data.str.sval : "";
            while(*p){
                while(*p==' '||*p=='\t')p++;
                if(!*p) break;
                char tok[PY_STR_MAX]; int ti=0;
                while(*p&&*p!=' '&&*p!='\t'&&ti<PY_STR_MAX-1) tok[ti++]=*p++;
                tok[ti]=0;
                if(lst->data.list.list_len < PY_LIST_MAX){

                    if(!lst->data.list.list){
                        lst->data.list.list = (py_val_t**)py_alloc(sizeof(py_val_t*)*PY_LIST_MAX);
                        if(!lst->data.list.list) break;
                        lst->data.list.list_cap = PY_LIST_MAX;
                    }
                    lst->data.list.list[lst->data.list.list_len++]=py_str_val(tok);
                }
            }
            return lst;
        }
        if(py_streq(method,"format")){

            char buf[PY_STR_MAX]; int bi=0,ai=0;
            const char *fmt=obj->data.str.sval ? obj->data.str.sval : "";
            while(*fmt && bi<PY_STR_MAX-1){
                if(fmt[0]=='{'&&fmt[1]=='}'){
                    if(ai<nargs){
                        char tmp[128]; py_val_to_str(args[ai++],tmp,128);
                        int ti=0; while(tmp[ti]&&bi<PY_STR_MAX-1) buf[bi++]=tmp[ti++];
                    }
                    fmt+=2;
                } else { buf[bi++]=*fmt++; }
            }
            buf[bi]=0; return py_str_val(buf);
        }
    }

    if(obj->type == PY_WIN){
        wapi_handle_t h = (wapi_handle_t)obj->data.ival;
        if(py_streq(method,"clear")){
            uint32_t col = nargs>0 ? (uint32_t)args[0]->data.ival : 0x101010;
            wapi_clear(h, col); return &PY_NONE_VAL;
        }
        if(py_streq(method,"fill_rect")){
            if(nargs>=5) wapi_fill_rect(h, args[0]->data.ival, args[1]->data.ival, args[2]->data.ival, args[3]->data.ival, (uint32_t)args[4]->data.ival);
            return &PY_NONE_VAL;
        }
        if(py_streq(method,"text")){
            if(nargs>=3){
                uint32_t fg = nargs>=4 ? (uint32_t)args[3]->data.ival : 0xFFFFFF;
                wapi_draw_text(h, args[0]->data.ival, args[1]->data.ival, args[2]->data.str.sval ? args[2]->data.str.sval : "", fg);
            }
            return &PY_NONE_VAL;
        }
        if(py_streq(method,"pixel")){
            if(nargs>=3) wapi_put_pixel(h, args[0]->data.ival, args[1]->data.ival, (uint32_t)args[2]->data.ival);
            return &PY_NONE_VAL;
        }
        if(py_streq(method,"flush")){
            wapi_flush(h); return &PY_NONE_VAL;
        }
        if(py_streq(method,"width")){
            return py_int_val(wapi_width(h));
        }
        if(py_streq(method,"height")){
            return py_int_val(wapi_height(h));
        }
        if(py_streq(method,"alive")){
            return py_bool_val(wapi_alive(h));
        }
        if(py_streq(method,"poll")){
            wapi_event_t ev;
            if(wapi_poll(h, &ev)){
                py_val_t *lst = py_list_val();
                if(lst->data.list.list_len < PY_LIST_MAX) lst->data.list.list[lst->data.list.list_len++] = py_int_val(ev.type);
                if(lst->data.list.list_len < PY_LIST_MAX) lst->data.list.list[lst->data.list.list_len++] = py_int_val(ev.type==WAPI_EV_KEY?(int)ev.key:ev.mx);
                if(lst->data.list.list_len < PY_LIST_MAX) lst->data.list.list[lst->data.list.list_len++] = py_int_val(ev.my);
                return lst;
            }
            return &PY_NONE_VAL;
        }
        if(py_streq(method,"destroy")){
            wapi_destroy(h); return &PY_NONE_VAL;
        }
    }
    terminal_printf("AttributeError: '%s' object has no attribute '%s'\n",
        obj->type==PY_STR?"str":obj->type==PY_LIST?"list":"object", method);
    return &PY_NONE_VAL;
}



static py_val_t *py_eval_primary(py_interp_t *interp, py_env_t *env, int *tp);
static py_val_t *py_eval_expr(py_interp_t *interp, py_env_t *env, int *tp);

static py_val_t *py_eval_primary(py_interp_t *interp, py_env_t *env, int *tp){
    py_tok_t *t = cur_tok(interp,*tp);
    py_val_t *val = &PY_NONE_VAL;


    if(t->type == TOK_NOT){
        consume(interp,tp);
        py_val_t *v = py_eval_primary(interp, env, tp);
        return py_bool_val(!py_truthy(v));
    }

    if(t->type == TOK_MINUS){
        consume(interp,tp);
        py_val_t *v = py_eval_primary(interp, env, tp);
        if(v->type==PY_FLOAT) return py_float_val(-v->data.fval);
        return py_int_val(-v->data.ival);
    }

    switch(t->type){
    case TOK_INT:   consume(interp,tp); val=py_int_val(t->ival); break;
    case TOK_FLOAT: consume(interp,tp); val=py_float_val(t->fval); break;
    case TOK_STR:   consume(interp,tp); val=py_str_val(t->val); break;
    case TOK_LPAREN:{
        consume(interp,tp);
        val=py_eval_expr(interp,env,tp);
        if(peek_type(interp,*tp)==TOK_RPAREN) consume(interp,tp);
        break;}
    case TOK_LBRAK:{

        consume(interp,tp);
        py_val_t *lst=py_list_val();
        while(peek_type(interp,*tp)!=TOK_RBRAK&&peek_type(interp,*tp)!=TOK_EOF){
            py_val_t *elem=py_eval_expr(interp,env,tp);
            if(lst->data.list.list_len<PY_LIST_MAX){

                if(!lst->data.list.list){
                    lst->data.list.list = (py_val_t**)py_alloc(sizeof(py_val_t*)*PY_LIST_MAX);
                    if(!lst->data.list.list){ val=lst; break; }
                    lst->data.list.list_cap = PY_LIST_MAX;
                }
                lst->data.list.list[lst->data.list.list_len++]=elem;
            }
            if(peek_type(interp,*tp)==TOK_COMMA) consume(interp,tp);
        }
        if(peek_type(interp,*tp)==TOK_RBRAK) consume(interp,tp);
        val=lst; break;}
    case TOK_NAME:{
        consume(interp,tp);
        const char *name = t->val;

        if(py_streq(name,"True"))  { val=&PY_TRUE_VAL; break; }
        if(py_streq(name,"False")) { val=&PY_FALSE_VAL; break; }
        if(py_streq(name,"None"))  { val=&PY_NONE_VAL; break; }


        if(peek_type(interp,*tp)==TOK_LPAREN){
            consume(interp,tp);
            py_val_t *args[16]; int nargs=0;
            while(peek_type(interp,*tp)!=TOK_RPAREN&&peek_type(interp,*tp)!=TOK_EOF){
                if(nargs<16) args[nargs++]=py_eval_expr(interp,env,tp);
                if(peek_type(interp,*tp)==TOK_COMMA) consume(interp,tp);
            }
            if(peek_type(interp,*tp)==TOK_RPAREN) consume(interp,tp);


            if(py_streq(name,"Window")){
                const char *title = nargs>0?(args[0]->data.str.sval ? args[0]->data.str.sval : ""):"Window";
                int x=nargs>1?args[1]->data.ival:100, y=nargs>2?args[2]->data.ival:100;
                int w=nargs>3?args[3]->data.ival:400, h=nargs>4?args[4]->data.ival:300;
                wapi_handle_t hdl=wapi_create(title,x,y,w,h);
                py_val_t *wv=py_alloc(sizeof(py_val_t));
                if(wv){ wv->type=PY_WIN; wv->data.ival=(int)hdl; val=wv; }
                break;
            }

            val=py_call_builtin(interp,env,name,args,nargs);
            break;
        }


        if(peek_type(interp,*tp)==TOK_DOT){
            py_val_t *obj = py_env_get(env, name);
            if(!obj){ terminal_printf("NameError: %s\n",name); interp->status=PY_RET_ERROR; break; }
            while(peek_type(interp,*tp)==TOK_DOT){
                consume(interp,tp);
                py_tok_t *mname = expect(interp,tp,TOK_NAME);
                if(interp->status) break;
                if(peek_type(interp,*tp)==TOK_LPAREN){
                    consume(interp,tp);
                    py_val_t *args[16]; int nargs=0;
                    while(peek_type(interp,*tp)!=TOK_RPAREN&&peek_type(interp,*tp)!=TOK_EOF){
                        if(nargs<16) args[nargs++]=py_eval_expr(interp,env,tp);
                        if(peek_type(interp,*tp)==TOK_COMMA) consume(interp,tp);
                    }
                    if(peek_type(interp,*tp)==TOK_RPAREN) consume(interp,tp);
                    obj = py_call_method(interp,obj,mname->val,args,nargs);
                } else {

                    obj = py_str_val(mname->val);
                }
            }
            val=obj; break;
        }


        if(peek_type(interp,*tp)==TOK_LBRAK){
            py_val_t *obj = py_env_get(env, name);
            if(!obj){ terminal_printf("NameError: %s\n",name); interp->status=PY_RET_ERROR; break; }
            consume(interp,tp);
            py_val_t *idx_v = py_eval_expr(interp,env,tp);
            if(peek_type(interp,*tp)==TOK_RBRAK) consume(interp,tp);
            int idx = idx_v->data.ival;
            if(obj->type==PY_LIST){
                if(idx<0) idx=obj->data.list.list_len+idx;
                if(obj->data.list.list && idx>=0&&idx<obj->data.list.list_len) val=obj->data.list.list[idx];
                else { terminal_printf("IndexError: list index out of range\n"); interp->status=PY_RET_ERROR; }
            } else if(obj->type==PY_STR){
                const char *sval = obj->data.str.sval ? obj->data.str.sval : "";
                if(idx<0) idx=py_slen(sval)+idx;
                if(idx>=0&&idx<(int)py_slen(sval)){
                    char buf[2]={sval[idx],0}; val=py_str_val(buf);
                } else { terminal_printf("IndexError: string index out of range\n"); interp->status=PY_RET_ERROR; }
            }
            break;
        }


        py_val_t *v = py_env_get(env, name);
        if(v) val=v;
        else {
            terminal_printf("NameError: name '%s' is not defined\n", name);
            interp->status=PY_RET_ERROR;
        }
        break;}
    default:
        break;
    }
    return val;
}


static int op_prec(int type){
    switch(type){
    case TOK_OR:      return 1;
    case TOK_AND:     return 2;
    case TOK_EQ: case TOK_NE:
    case TOK_LT: case TOK_LE:
    case TOK_GT: case TOK_GE: return 3;
    case TOK_PLUS: case TOK_MINUS: return 4;
    case TOK_STAR: case TOK_SLASH:
    case TOK_PERCENT: return 5;
    default: return 0;
    }
}

static py_val_t *py_eval_expr_prec(py_interp_t *interp, py_env_t *env, int *tp, int min_prec){
    py_val_t *left = py_eval_primary(interp,env,tp);
    while(1){
        int tt = peek_type(interp,*tp);
        int prec = op_prec(tt);
        if(prec < min_prec || prec == 0) break;
        consume(interp,tp);
        py_val_t *right = py_eval_expr_prec(interp,env,tp, prec+1);

        int li=left->data.ival, ri=right->data.ival;
        float lf=left->data.fval, rf=right->data.fval;
        int is_float=(left->type==PY_FLOAT||right->type==PY_FLOAT);
        switch(tt){
        case TOK_PLUS:
            if(left->type==PY_STR||right->type==PY_STR){
                char buf[PY_STR_MAX]; char lbs[PY_STR_MAX],rbs[PY_STR_MAX];
                py_val_to_str(left,lbs,PY_STR_MAX); py_val_to_str(right,rbs,PY_STR_MAX);
                int ll=py_slen(lbs),rl=py_slen(rbs);
                int i=0; while(i<ll&&i<PY_STR_MAX-1)buf[i]=lbs[i++]; while(rbs[i-ll]&&i<PY_STR_MAX-1)buf[i]=rbs[i-ll],i++; buf[i]=0;
                left=py_str_val(buf);
            } else if(is_float) left=py_float_val(lf+rf);
            else left=py_int_val(li+ri);
            break;
        case TOK_MINUS: if(is_float)left=py_float_val(lf-rf); else left=py_int_val(li-ri); break;
        case TOK_STAR:  if(is_float)left=py_float_val(lf*rf); else left=py_int_val(li*ri); break;
        case TOK_SLASH: if(rf==0.0f&&ri==0){left=py_float_val(0);terminal_printf("ZeroDivisionError\n");}
                        else if(is_float)left=py_float_val(lf/rf); else left=py_float_val((float)li/(float)ri); break;
        case TOK_PERCENT: if(ri==0)left=py_int_val(0); else left=py_int_val(li%ri); break;
        case TOK_EQ:  left=py_bool_val(is_float?(lf==rf):(li==ri)); break;
        case TOK_NE:  left=py_bool_val(is_float?(lf!=rf):(li!=ri)); break;
        case TOK_LT:  left=py_bool_val(is_float?(lf<rf):(li<ri)); break;
        case TOK_LE:  left=py_bool_val(is_float?(lf<=rf):(li<=ri)); break;
        case TOK_GT:  left=py_bool_val(is_float?(lf>rf):(li>ri)); break;
        case TOK_GE:  left=py_bool_val(is_float?(lf>=rf):(li>=ri)); break;
        case TOK_AND: left=py_bool_val(py_truthy(left)&&py_truthy(right)); break;
        case TOK_OR:  left=py_bool_val(py_truthy(left)||py_truthy(right)); break;
        default: break;
        }
    }
    return left;
}

static py_val_t *py_eval_expr(py_interp_t *interp, py_env_t *env, int *tp){
    return py_eval_expr_prec(interp,env,tp,1);
}


static int py_exec_stmts(py_interp_t *interp, py_env_t *env, int *tp, int dedent_stop){
    while(interp->status == PY_RET_NORMAL){
        int tt = peek_type(interp,*tp);
        if(tt == TOK_EOF) break;
        if(dedent_stop && tt == TOK_DEDENT){ consume(interp,tp); break; }
        if(tt == TOK_NEWLINE){ consume(interp,tp); continue; }
        if(tt == TOK_INDENT){ consume(interp,tp); continue; }
        if(tt == TOK_PASS)  { consume(interp,tp); if(peek_type(interp,*tp)==TOK_NEWLINE)consume(interp,tp); continue; }
        if(tt == TOK_DEDENT){ consume(interp,tp); continue; }

        py_tok_t *t = cur_tok(interp,*tp);


        if(tt==TOK_NAME && py_streq(t->val,"import")){
            consume(interp,tp);
            py_tok_t *mod = expect(interp,tp,TOK_NAME);
            if(interp->status) return interp->status;

            (void)mod;
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            continue;
        }


        if(tt==TOK_NAME && py_streq(t->val,"from")){
            skip_to_newline(interp,tp);
            continue;
        }


        if(tt==TOK_NAME && py_streq(t->val,"def")){
            consume(interp,tp);
            py_tok_t *fname = expect(interp,tp,TOK_NAME);
            if(interp->status) return interp->status;
            expect(interp,tp,TOK_LPAREN);
            py_val_t *fn = (py_val_t*)py_alloc(sizeof(py_val_t));
            if(!fn){ interp->status=PY_RET_ERROR; return interp->status; }
            fn->type=PY_FUNC; fn->data.func.param_count=0;
            while(peek_type(interp,*tp)!=TOK_RPAREN&&peek_type(interp,*tp)!=TOK_EOF){
                py_tok_t *pname=expect(interp,tp,TOK_NAME);
                if(fn->data.func.param_count<8){
                    char *pn=(char*)py_alloc(PY_VAR_NAMELEN);
                    if(pn){ py_scpy(pn,pname->val,PY_VAR_NAMELEN); fn->data.func.param_names[fn->data.func.param_count++]=pn; }
                }
                if(peek_type(interp,*tp)==TOK_COMMA) consume(interp,tp);
            }
            expect(interp,tp,TOK_RPAREN);
            expect(interp,tp,TOK_COLON);
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            fn->data.func.func_tok_start = *tp;

            py_env_set(env, fname->val, fn);

            skip_block(interp,tp);
            continue;
        }


        if(tt==TOK_NAME && py_streq(t->val,"return")){
            consume(interp,tp);
            if(peek_type(interp,*tp)!=TOK_NEWLINE&&peek_type(interp,*tp)!=TOK_EOF)
                interp->return_val = py_eval_expr(interp,env,tp);
            else
                interp->return_val = &PY_NONE_VAL;
            interp->status = PY_RET_RETURN;
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            return PY_RET_RETURN;
        }


        if(tt==TOK_NAME && py_streq(t->val,"break")){
            consume(interp,tp);
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            interp->status=PY_RET_BREAK; return PY_RET_BREAK;
        }
        if(tt==TOK_NAME && py_streq(t->val,"continue")){
            consume(interp,tp);
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            interp->status=PY_RET_CONTINUE; return PY_RET_CONTINUE;
        }


        if(tt==TOK_NAME && (py_streq(t->val,"if")||py_streq(t->val,"elif"))){
            consume(interp,tp);
            py_val_t *cond = py_eval_expr(interp,env,tp);
            expect(interp,tp,TOK_COLON);
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            if(py_truthy(cond)){
                py_exec_stmts(interp,env,tp,1);

                while(peek_type(interp,*tp)==TOK_NAME){
                    py_tok_t *nxt=cur_tok(interp,*tp);
                    if(py_streq(nxt->val,"elif")||py_streq(nxt->val,"else")){
                        skip_to_newline(interp,tp);
                        if(peek_type(interp,*tp)==TOK_INDENT) skip_block(interp,tp);
                    } else break;
                }
            } else {
                skip_block(interp,tp);

                while(peek_type(interp,*tp)==TOK_NAME){
                    py_tok_t *nxt=cur_tok(interp,*tp);
                    if(py_streq(nxt->val,"elif")){

                        continue;
                    } else if(py_streq(nxt->val,"else")){
                        consume(interp,tp);
                        expect(interp,tp,TOK_COLON);
                        if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
                        py_exec_stmts(interp,env,tp,1);
                    }
                    break;
                }
            }
            continue;
        }


        if(tt==TOK_NAME && py_streq(t->val,"while")){
            consume(interp,tp);
            int cond_tp = *tp;
            py_val_t *cond = py_eval_expr(interp,env,tp);
            expect(interp,tp,TOK_COLON);
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            int body_tp = *tp;
            if(!py_truthy(cond)){ skip_block(interp,tp); continue; }
            while(py_truthy(cond) && interp->status==PY_RET_NORMAL){
                int loop_tp = body_tp;
                py_exec_stmts(interp,env,&loop_tp,1);
                if(interp->status==PY_RET_BREAK){ interp->status=PY_RET_NORMAL; break; }
                if(interp->status==PY_RET_CONTINUE){ interp->status=PY_RET_NORMAL; }

                int ctmp = cond_tp;
                cond = py_eval_expr(interp,env,&ctmp);
                *tp = loop_tp;
            }
            if(*tp == body_tp) skip_block(interp,tp);
            continue;
        }


        if(tt==TOK_NAME && py_streq(t->val,"for")){
            consume(interp,tp);
            py_tok_t *var = expect(interp,tp,TOK_NAME);

            py_tok_t *inkw = consume(interp,tp);
            (void)inkw;
            py_val_t *iterable = py_eval_expr(interp,env,tp);
            expect(interp,tp,TOK_COLON);
            if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
            int body_tp = *tp;
            if(!iterable||iterable->type!=PY_LIST){ skip_block(interp,tp); continue; }
            for(int ii=0; ii<iterable->data.list.list_len && interp->status==PY_RET_NORMAL; ii++){
                py_env_set(env, var->val, iterable->data.list.list[ii]);
                int loop_tp = body_tp;
                py_exec_stmts(interp,env,&loop_tp,1);
                if(interp->status==PY_RET_BREAK){ interp->status=PY_RET_NORMAL; break; }
                if(interp->status==PY_RET_CONTINUE){ interp->status=PY_RET_NORMAL; }
                *tp = loop_tp;
            }
            if(*tp == body_tp) skip_block(interp,tp);
            continue;
        }



        if(tt==TOK_NAME){

            int next_tt = peek_type(interp,*tp+1);
            if(next_tt==TOK_ASSIGN){

                py_tok_t *vname = consume(interp,tp);
                consume(interp,tp);
                py_val_t *val = py_eval_expr(interp,env,tp);
                py_env_set(env, vname->val, val);
                if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
                continue;
            }
            if(next_tt==TOK_PLUSEQ||next_tt==TOK_MINUSEQ||next_tt==TOK_STAREQ||next_tt==TOK_SLASHEQ){
                py_tok_t *vname = consume(interp,tp);
                int op=consume(interp,tp)->type;
                py_val_t *rhs = py_eval_expr(interp,env,tp);
                py_val_t *lhs = py_env_get(env,vname->val);
                if(!lhs) lhs=py_int_val(0);
                py_val_t *res;
                if(op==TOK_PLUSEQ) res=py_int_val(lhs->data.ival+rhs->data.ival);
                else if(op==TOK_MINUSEQ) res=py_int_val(lhs->data.ival-rhs->data.ival);
                else if(op==TOK_STAREQ) res=py_int_val(lhs->data.ival*rhs->data.ival);
                else res=rhs->data.ival?py_float_val((float)lhs->data.ival/(float)rhs->data.ival):py_float_val(0);
                py_env_set(env,vname->val,res);
                if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
                continue;
            }

            if(next_tt==TOK_LBRAK){
                py_tok_t *vname=consume(interp,tp);
                consume(interp,tp);
                py_val_t *idx_v=py_eval_expr(interp,env,tp);
                expect(interp,tp,TOK_RBRAK);
                if(interp->status) return interp->status;
                if(peek_type(interp,*tp)==TOK_ASSIGN){
                    consume(interp,tp);
                    py_val_t *rhs=py_eval_expr(interp,env,tp);
                    py_val_t *obj=py_env_get(env,vname->val);
                    if(obj&&obj->type==PY_LIST){
                        int idx=idx_v->data.ival; if(idx<0)idx=obj->data.list.list_len+idx;
                        if(obj->data.list.list && idx>=0&&idx<obj->data.list.list_len) obj->data.list.list[idx]=rhs;
                    }
                    if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
                    continue;
                }


            }
        }


        py_eval_expr(interp,env,tp);
        if(peek_type(interp,*tp)==TOK_NEWLINE) consume(interp,tp);
    }
    return interp->status;
}



int python_exec(const char *source){
    if(!source) return 1;


    int cap = MAX_TOKENS;
    py_tok_t *toks = (py_tok_t*)py_alloc(sizeof(py_tok_t) * (size_t)cap);
    if(!toks){ terminal_printf("python: out of memory\n"); return 1; }

    py_lex_t lx = {source, 0, 1, toks, 0, cap};
    lex_run(&lx);

    py_interp_t interp;
    interp.toks      = toks;
    interp.ntoks     = lx.ntoks;
    interp.return_val= NULL;
    interp.status    = PY_RET_NORMAL;
    interp.errmsg[0] = 0;

    py_env_t *env = py_env_push(NULL);

    int tp = 0;
    py_exec_stmts(&interp, env, &tp, 0);

    int err = (interp.status == PY_RET_ERROR) ? 1 : 0;

    py_env_pop(env);
    py_free(toks);
    return err;
}

int python_exec_file(const char *path){
    sfs_node_t *root = sfs_root();
    sfs_node_t *node = sfs_resolve(root, path);
    if(!node || node->type != SFS_FILE || !node->data){
        terminal_printf("python: cannot open '%s'\n", path);
        return 1;
    }

    size_t sz = (size_t)node->size;
    char *src = (char*)py_alloc(sz + 1);
    if(!src){ terminal_printf("python: out of memory\n"); return 1; }
    for(size_t i=0;i<sz;i++) src[i]=((char*)node->data)[i];
    src[sz]=0;
    int r = python_exec(src);
    py_free(src);
    return r;
}

void python_repl(void){
    terminal_printf("stinger Python 3.x (subset)\nType 'exit()' to quit.\n");
    char line[256];
    while(1){
        terminal_writestring(">>> ");
        int li=0; char c;
        while((c=keyboard_getchar())!='\n'&&c!='\r'&&li<255){
            terminal_putchar(c); line[li++]=c;
        }
        line[li]=0; terminal_putchar('\n');
        if(li==0) continue;
        if(py_streq(line,"exit()")||py_streq(line,"quit()")) break;
        python_exec(line);
    }
}




