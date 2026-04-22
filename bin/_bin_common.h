












static inline int b_slen(const char *s) { int n=0; while(s[n]) n++; return n; }
static inline int b_scmp(const char *a, const char *b) {
    while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b;
}
static inline int b_sncmp(const char *a, const char *b, int n) {
    while(n&&*a&&*a==*b){a++;b++;n--;} return n?((unsigned char)*a-(unsigned char)*b):0;
}
static inline void b_scpy(char *d, const char *s) { while((*d++=*s++)); }
static inline void b_sncpy(char *d, const char *s, int n) {
    int i=0; while(i<n-1&&s[i]){d[i]=s[i];i++;} d[i]=0;
}
static inline void b_memset(void *d, int v, int n) {
    unsigned char *p=d; while(n--)*p++=(unsigned char)v;
}
static inline void b_memcpy(void *d, const void *s, int n) {
    unsigned char *dd=d; const unsigned char *ss=s; while(n--)*dd++=*ss++;
}
static inline int b_memcmp(const void *a, const void *b, int n) {
    const unsigned char *aa=a,*bb=b;
    while(n--){if(*aa!=*bb)return *aa-*bb;aa++;bb++;}return 0;
}
static inline int b_isspace(char c){return c==' '||c=='\t'||c=='\n'||c=='\r';}
static inline int b_isdigit(char c){return c>='0'&&c<='9';}
static inline int b_isalpha(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z');}
static inline int b_islower(char c){return c>='a'&&c<='z';}
static inline int b_isupper(char c){return c>='A'&&c<='Z';}
static inline char b_tolower(char c){return b_isupper(c)?(char)(c+32):c;}
static inline char b_toupper(char c){return b_islower(c)?(char)(c-32):c;}

static inline int b_atoi(const char *s) {
    int n=0,neg=0;
    while(b_isspace(*s))s++;
    if(*s=='-'){neg=1;s++;}else if(*s=='+')s++;
    while(b_isdigit(*s))n=n*10+(*s++)-'0';
    return neg?-n:n;
}
static inline unsigned long b_atoul(const char *s) {
    unsigned long n=0;
    while(b_isspace(*s))s++;
    if(s[0]=='0'&&(s[1]=='x'||s[1]=='X')){
        s+=2;
        while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F')){
            n<<=4;
            if(*s>='0'&&*s<='9')n+=*s-'0';
            else if(*s>='a'&&*s<='f')n+=*s-'a'+10;
            else n+=*s-'A'+10;
            s++;
        }
    } else {
        while(b_isdigit(*s))n=n*10+(*s++)-'0';
    }
    return n;
}


static inline char *b_uitoa(unsigned int v, char *buf) {
    if(v==0){*buf++='0';*buf=0;return buf;}
    char tmp[12]; int i=0;
    while(v){tmp[i++]='0'+v%10;v/=10;}
    char *p=buf;
    while(i>0)*p++=tmp[--i];
    *p=0; return p;
}
static inline char *b_itoa_s(int v, char *buf) {
    if(v<0){*buf++='-';v=-v;}
    return b_uitoa((unsigned int)v,buf);
}
static inline char *b_utohex(unsigned int v, char *buf, int upper) {
    const char *h=upper?"0123456789ABCDEF":"0123456789abcdef";
    if(v==0){*buf++='0';*buf=0;return buf;}
    char tmp[12]; int i=0;
    while(v){tmp[i++]=h[v&0xF];v>>=4;}
    char *p=buf;
    while(i>0)*p++=tmp[--i];
    *p=0; return p;
}


static inline void b_printf(const char *fmt, ...) {

    __builtin_va_list ap; __builtin_va_start(ap,fmt);
    char buf[32];
    while(*fmt){
        if(*fmt!='%'){terminal_putchar(*fmt++);continue;}
        fmt++;
        int left=0,zero=0,width=0;
        if(*fmt=='-'){left=1;fmt++;}
        if(*fmt=='0'){zero=1;fmt++;}
        while(b_isdigit(*fmt))width=width*10+(*fmt++)-'0';
        switch(*fmt++){
        case 'd':{
            int v=__builtin_va_arg(ap,int);
            b_itoa_s(v,buf);
            int l=b_slen(buf);
            if(!left)while(l++<width)terminal_putchar(zero?'0':' ');
            terminal_writestring(buf);
            if(left)while(l++<width)terminal_putchar(' ');
            break;}
        case 'u':{
            unsigned v=__builtin_va_arg(ap,unsigned);
            b_uitoa(v,buf);
            int l=b_slen(buf);
            if(!left)while(l++<width)terminal_putchar(zero?'0':' ');
            terminal_writestring(buf);
            if(left)while(l++<width)terminal_putchar(' ');
            break;}
        case 'x':case 'X':{
            unsigned v=__builtin_va_arg(ap,unsigned);
            b_utohex(v,buf,*(fmt-1)=='X');
            int l=b_slen(buf);
            if(!left)while(l++<width)terminal_putchar(zero?'0':' ');
            terminal_writestring(buf);
            if(left)while(l++<width)terminal_putchar(' ');
            break;}
        case 's':{
            const char *s=__builtin_va_arg(ap,const char*);
            if(!s)s="(null)";
            int l=b_slen(s);
            if(!left)while(l++<width)terminal_putchar(' ');
            terminal_writestring(s);
            if(left)while(l++<width)terminal_putchar(' ');
            break;}
        case 'c':terminal_putchar((char)__builtin_va_arg(ap,int));break;
        case '%':terminal_putchar('%');break;
        default: terminal_putchar(*(fmt-1));break;
        }
    }
    __builtin_va_end(ap);
}





static inline sfs_node_t *b_resolve(sfs_node_t *cwd, const char *path) {
    if(!path||!path[0]) return cwd;
    return sfs_resolve(cwd, path);
}


static inline int b_read_file(sfs_node_t *cwd, const char *path,
                               uint8_t **out, size_t *sz) {
    sfs_node_t *n = b_resolve(cwd, path);
    if(!n||n->type!=SFS_FILE||!n->data||!n->size) return -1;
    *out=(uint8_t*)kmalloc(n->size+1);
    if(!*out) return -1;
    b_memcpy(*out,n->data,(int)n->size);
    (*out)[n->size]=0;
    *sz=n->size;
    return 0;
}


static inline void b_each_line(char *buf, size_t sz,
                                void (*cb)(char*line, void*ctx), void *ctx) {
    size_t i=0;
    while(i<sz){
        char *line=buf+i;
        while(i<sz&&buf[i]!='\n')i++;
        if(i<sz)buf[i]=0;
        cb(line,ctx);
        if(i<sz){buf[i]='\n';i++;}
    }
}


static inline int b_count_lines(const char *buf, size_t sz) {
    int n=0; for(size_t i=0;i<sz;i++) if(buf[i]=='\n') n++;
    return n;
}


static inline int b_wrap(const char *s, int width) {
    if(b_slen(s)<=width) return b_slen(s);
    int last_sp=0;
    for(int i=0;i<width;i++){if(s[i]==' ')last_sp=i;}
    return last_sp?last_sp:width;
}






