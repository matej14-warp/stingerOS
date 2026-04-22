





















int g_in_yaoigui = 0;
void yaoigui_crash_exit(void){
    g_in_yaoigui=0; vbe_set_autoflip(1); vbe_terminal_clear();
    terminal_printf("[yaoigui] crashed - back to terminal\n");
    extern void shell_run(void); shell_run();
}


static inline void _outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t _inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
uint32_t paging_get_phys(uint32_t virt_addr);
static uint32_t _ticks(void){extern uint32_t get_ticks(void);return get_ticks();}


void cmd_snake_gui(int x, int y, int w, int h);
void cmd_tetris_gui(int x, int y, int w, int h);
void cmd_pong_gui(int x, int y, int w, int h);
void cmd_minecraft_gui(int x, int y, int w, int h);


void *memcpy(void *d,const void *s,size_t n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;return d;}
void *memset(void *d,int c,size_t n){uint8_t*dd=d;while(n--)*dd++=(uint8_t)c;return d;}
static int _slen(const char*s){int n=0;while(*s++)n++;return n;}
static void _pad2(char*b,int v){b[0]=(char)('0'+v/10%10);b[1]=(char)('0'+v%10);b[2]=0;}
static void _itoa(char*b,unsigned v){if(!v){b[0]='0';b[1]=0;return;}char t[12];int i=0;while(v){t[i++]=(char)('0'+v%10);v/=10;}int j=0;while(i--)b[j++]=t[i];b[j]=0;}
static void _scpy(char*d,const char*s,int n){int i=0;while(s[i]&&i<n-1){d[i]=s[i];i++;}d[i]=0;}
static void _scat(char*d,const char*s,int n){int i=0;while(d[i])i++;int j=0;while(s[j]&&i+j<n-1){d[i+j]=s[j];j++;}d[i+j]=0;}
static int pin(int px,int py,int x,int y,int w,int h){return px>=x&&px<x+w&&py>=y&&py<y+h;}





















































static void dr(int x,int y,int w,int h,uint32_t c){vbe_fill_rect(x,y,w,h,c);}
static void dbdr(int x,int y,int w,int h,uint32_t c){
    vbe_fill_rect(x,y,w,1,c);vbe_fill_rect(x,y+h-1,w,1,c);
    vbe_fill_rect(x,y,1,h,c);vbe_fill_rect(x+w-1,y,1,h,c);
}
static void ds(int x,int y,const char*s,uint32_t fg,uint32_t bg){
    while(*s){vbe_draw_char(x,y,*s++,fg,bg);x+=FW;}
}
static void dsn(int x,int y,const char*s,int n,uint32_t fg,uint32_t bg){
    for(int i=0;i<n&&s[i];i++){vbe_draw_char(x,y,s[i],fg,bg);x+=FW;}
}



static uint32_t *g_wallpaper = NULL;
static int g_wp_w=0, g_wp_h=0;

extern volatile int mouse_x, mouse_y;
volatile int mouse_buttons = 0;




static void gui_load_wallpaper(const char *path) {
    if (!path) return;
    char fullpath[280];
    if (path[0] == '/') {
        _scpy(fullpath, path, 280);
    } else {
        _scpy(fullpath, "/wallpapers/", 13);
        _scat(fullpath, path, 280);
    }

    sfs_node_t *n = sfs_resolve(sfs_root(), fullpath);
    if (!n || n->type != SFS_FILE) return;


    const char *data = (const char*)n->data;
    if (data[0] != 'P' || data[1] != '6') return;


    size_t pos = 3;
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
    int w = 0; while(pos < n->size && data[pos] >= '0' && data[pos] <= '9') { w = w*10 + (data[pos]-'0'); pos++; }
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
    int h = 0; while(pos < n->size && data[pos] >= '0' && data[pos] <= '9') { h = h*10 + (data[pos]-'0'); pos++; }
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
    while(pos < n->size && data[pos] >= '0' && data[pos] <= '9') pos++;
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;

    if (w <= 0 || h <= 0 || pos >= n->size) return;

    if (g_wallpaper) kfree(g_wallpaper);
    g_wallpaper = (uint32_t*)kmalloc(w * h * 4);
    if (!g_wallpaper) return;
    g_wp_w = w; g_wp_h = h;


    for (int i = 0; i < w * h; i++) {
        if (pos + 3 > n->size) break;
        uint8_t r = data[pos++];
        uint8_t g = data[pos++];
        uint8_t b = data[pos++];
        g_wallpaper[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}

static void draw_background(void) {
    int sw = vbe_screen_width, sh = vbe_screen_height;
    int th = TBAR_H;

    if (g_wallpaper) {
        int x = (sw - g_wp_w) / 2;
        int y = (sh - th - g_wp_h) / 2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;


        if (g_wp_w < sw || g_wp_h < (sh - th)) {
            dr(0, 0, sw, sh - th, C_DESK);
        }

        vbe_bb_blit_rect(x, y,
            (g_wp_w > sw) ? sw : g_wp_w,
            (g_wp_h > sh - th) ? (sh - th) : g_wp_h,
            g_wallpaper);
    } else {
        dr(0, 0, sw, sh - th, C_DESK);
    }
}

typedef struct{int h,m,s;}rtc_t;
static int _bcd(uint8_t v){return(int)((v>>4)*10+(v&0xF));}
static rtc_t rtc_now(void){
    while(_inb(0x70),_inb(0x71)&0x80);
    _outb(0x70,0x00);uint8_t s=_inb(0x71);
    _outb(0x70,0x02);uint8_t mn=_inb(0x71);
    _outb(0x70,0x04);uint8_t h=_inb(0x71);
    _outb(0x70,0x0B);uint8_t b=_inb(0x71);
    rtc_t t;
    if(b&0x04){t.s=s;t.m=mn;t.h=h&0x7F;}
    else{t.s=_bcd(s);t.m=_bcd(mn);t.h=_bcd(h&0x7F);}
    if(!(b&0x02)&&(h&0x80))t.h=(t.h+12)%24;
    return t;
}


typedef struct {
    uint8_t  px[16][16];
    uint32_t pal[8];
    int      atlas_x, atlas_y;
} icon_t;

static uint32_t *g_icon_atlas = NULL;
static uint32_t  g_icon_atlas_phys = 0;

static void dricon(int x,int y,const icon_t *ic){
    for(int r=0;r<16;r++){
        int py=y+r;
        if(py<0||py>=vbe_screen_height)continue;
        uint32_t *row=vbe_bb_row(py);
        for(int c2=0;c2<16;c2++){
            uint8_t pi=ic->px[r][c2]; if(!pi) continue;
            int px=x+c2;
            if(px<0||px>=vbe_screen_width)continue;
            if(pi>7)pi=7;
            row[px]=ic->pal[pi-1];
        }
    }
}


static const icon_t ICO_SCORP = {
    .px={
        {0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0},
        {0,0,1,2,2,2,2,1,0,0,0,0,0,0,0,0},
        {0,1,2,2,1,1,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,1,1,1,1,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
        {0,1,2,2,2,2,2,2,2,2,2,1,0,0,0,0},
        {0,0,1,1,2,2,2,2,2,1,1,0,0,0,0,0},
        {0,0,0,1,2,2,2,2,1,0,0,1,1,0,0,0},
        {0,0,0,0,1,2,2,1,0,0,1,2,2,1,0,0},
        {0,0,0,0,0,1,1,0,0,0,1,2,3,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,1,2,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
        {0},{0},{0},
    },
    .pal={C(0,160,20),C(0,220,60),C(255,230,0),0,0,0,0}
};


static const icon_t ICO_TERM = {
    .px={
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,3,3,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,3,3,3,2,2,2,2,2,2,2,2,1},
        {1,2,3,3,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,4,4,4,4,4,4,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,4,4,4,4,4,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    },
    .pal={C(20,20,30),C(20,20,30),C(0,220,80),C(0,180,60),0,0,0}
};


static const icon_t ICO_FOLDER = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(200,160,0),C(255,210,50),0,0,0,0,0}
};


static const icon_t ICO_BROWSER = {
    .px={
        {0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0},
        {0,0,1,2,2,2,2,2,2,2,2,2,1,0,0,0},
        {0,1,2,2,1,2,2,2,2,2,1,2,2,1,0,0},
        {1,2,2,1,1,1,2,2,2,1,1,1,2,2,1,0},
        {1,2,2,2,1,1,1,2,1,1,1,2,2,2,1,0},
        {1,2,2,2,2,1,2,2,2,1,2,2,2,2,1,0},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,0},
        {1,2,2,2,1,2,2,2,2,2,1,2,2,2,1,0},
        {1,2,2,2,1,1,2,2,2,1,1,2,2,2,1,0},
        {1,2,2,1,1,1,1,2,1,1,1,1,2,2,1,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {0,0,1,2,2,2,2,2,2,2,2,2,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(0,100,200),C(50,160,255),0,0,0,0,0}
};


static const icon_t ICO_SETTINGS = {
    .px={
        {0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0},
        {0,0,0,1,1,2,2,2,2,2,1,1,0,0,0,0},
        {0,0,1,1,2,2,2,2,2,2,2,1,1,0,0,0},
        {0,1,1,2,2,2,3,3,3,2,2,2,1,1,0,0},
        {1,1,2,2,2,3,3,3,3,3,2,2,2,1,1,0},
        {1,1,2,2,3,3,3,2,3,3,3,2,2,1,1,0},
        {1,1,2,2,3,3,2,2,2,3,3,2,2,1,1,0},
        {0,1,2,2,3,3,2,2,2,3,3,2,2,1,0,0},
        {0,1,2,2,3,3,2,2,2,3,3,2,2,1,0,0},
        {1,1,2,2,3,3,3,2,3,3,3,2,2,1,1,0},
        {1,1,2,2,2,3,3,3,3,3,2,2,2,1,1,0},
        {0,1,1,2,2,2,3,3,3,2,2,2,1,1,0,0},
        {0,0,1,1,2,2,2,2,2,2,2,1,1,0,0,0},
        {0,0,0,1,1,2,2,2,2,2,1,1,0,0,0,0},
        {0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(100,100,100),C(200,200,200),C(80,180,255),0,0,0,0}
};


static const icon_t ICO_SYSINFO = {
    .px={
        {0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
        {0,0,1,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {1,1,1,2,3,3,3,3,3,3,3,3,2,1,1,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,2,2,2,3,4,4,4,4,4,4,3,2,2,2,1},
        {1,1,1,2,3,3,3,3,3,3,3,3,2,1,1,1},
        {0,0,1,2,2,2,2,2,2,2,2,2,2,1,0,0},
        {0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
        {0},{0},
    },
    .pal={C(80,80,80),C(140,140,140),C(60,180,255),C(30,100,200),0,0,0}
};


static const icon_t ICO_NET = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,7,7,1,1,0,0,0,0,0},
        {0,0,0,0,1,7,1,7,7,1,7,1,0,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,1,7,7,1,7,7,1,7,7,1,0,0,0},
        {0,0,0,1,7,7,1,7,7,1,7,7,1,0,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,0,1,7,1,7,7,1,7,1,0,0,0,0},
        {0,0,0,0,0,1,1,7,7,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(0,0,0), C(0,160,20), C(0,220,60), C(255,230,0), C(255,80,80), C(80,160,255), C(255,255,255), C(128,128,128)}
};


static const icon_t ICO_GAME = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,1,1,7,7,7,7,7,7,1,1,0,0,0},
        {0,0,1,7,7,1,7,7,7,7,1,7,0,1,0,0},
        {0,0,1,7,1,1,1,7,7,1,7,1,7,1,0,0},
        {0,0,1,7,7,1,7,7,7,7,1,7,7,1,0,0},
        {0,0,1,7,7,7,7,7,7,7,7,7,7,1,0,0},
        {0,1,7,7,7,7,1,1,1,1,7,7,7,7,1,0},
        {0,1,7,7,7,1,0,0,0,0,1,7,7,7,1,0},
        {0,1,1,7,1,1,0,0,0,0,1,1,7,1,1,0},
        {0,0,1,1,1,0,0,0,0,0,0,1,1,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(0,0,0), C(0,160,20), C(0,220,60), C(255,230,0), C(255,80,80), C(80,160,255), C(255,255,255), C(128,128,128)}
};


static const icon_t ICO_TOOLS= {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,0},
        {0,1,0,0,1,1,0,0,0,0,0,0,1,1,0,0},
        {0,1,1,1,1,1,0,0,0,0,0,1,1,1,0,0},
        {0,0,1,1,1,1,1,0,0,0,1,1,0,0,0,0},
        {0,0,0,0,0,1,1,1,0,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,7,1,1,1,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0},
        {0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0},
        {0,0,0,1,1,1,1,1,0,0,1,1,7,1,1,0},
        {0,0,1,1,1,1,1,0,0,0,1,7,7,7,1,0},
        {0,1,1,1,1,1,0,0,0,0,1,1,7,1,1,0},
        {0,0,1,1,1,0,0,0,0,0,0,1,1,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(0,0,0), C(0,160,20), C(0,220,60), C(255,230,0), C(255,80,80), C(80,160,255), C(255,255,255), C(128,128,128)}
};


static const icon_t ICO_POWER = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
        {0,0,0,1,1,7,7,1,1,7,7,1,1,0,0,0},
        {0,0,1,1,1,7,7,1,1,7,7,1,1,1,0,0},
        {0,0,1,1,7,7,7,1,1,7,7,7,1,1,0,0},
        {0,1,1,7,7,7,7,1,1,7,7,7,7,1,1,0},
        {0,1,1,7,7,7,7,7,7,7,7,7,7,1,1,0},
        {0,1,1,7,7,7,7,7,7,7,7,7,7,1,1,0},
        {0,1,1,7,7,7,7,7,7,7,7,7,7,1,1,0},
        {0,1,1,7,7,7,7,7,7,7,7,7,7,1,1,0},
        {0,0,1,1,7,7,7,7,7,7,7,7,1,1,0,0},
        {0,0,1,1,1,7,7,7,7,7,0,1,1,1,0,0},
        {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
        {0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    },
    .pal={C(0,0,0), C(0,160,20), C(0,220,60), C(255,230,0), C(255,80,80), C(80,160,255), C(255,255,255), C(128,128,128)}
};


static const icon_t ICO_GCC = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
        {0,1,2,3,3,3,3,3,3,3,3,3,3,2,1,0},
        {0,1,2,3,4,0,0,0,0,0,0,4,3,2,1,0},
        {0,1,2,3,0,4,4,4,4,4,0,3,3,2,1,0},
        {0,1,2,3,0,4,5,5,5,4,0,3,3,2,1,0},
        {0,1,2,3,0,4,5,5,5,4,0,3,3,2,1,0},
        {0,1,2,3,0,4,5,5,5,4,0,3,3,2,1,0},
        {0,1,2,3,4,0,0,4,4,0,4,3,3,2,1,0},
        {0,1,2,3,3,3,0,0,0,3,3,3,3,2,1,0},
        {0,1,2,2,2,3,3,3,3,3,2,2,2,2,1,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0},{0},
    },
    .pal={C(30,30,30),C(50,50,50),C(80,80,80),C(0,180,80),C(0,230,100),0,0}
};


static const icon_t ICO_NANO = {
    .px={
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,3,3,3,3,3,3,3,3,3,3,3,3,2,1},
        {1,2,3,4,4,4,4,4,4,4,4,4,3,3,2,1},
        {1,2,3,4,4,4,4,4,4,4,4,4,3,3,2,1},
        {1,2,3,3,3,3,3,3,3,3,3,3,3,3,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,4,4,4,4,4,4,4,4,4,4,4,4,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,4,4,4,4,4,4,4,4,4,4,4,4,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,4,4,4,4,4,4,4,4,4,4,4,4,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    },
    .pal={C(80,80,120),C(100,100,160),C(120,120,200),C(200,220,255),0,0,0,0}
};


static const icon_t ICO_DRAW = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,2,2,2,1,0},
        {0,0,0,0,0,0,0,0,0,1,2,2,1,0,0,0},
        {0,0,0,0,0,0,0,0,1,2,2,1,0,0,0,0},
        {0,0,0,0,0,0,0,1,3,3,1,0,0,0,0,0},
        {0,0,0,0,0,0,1,3,3,1,0,0,0,0,0,0},
        {0,0,0,0,0,1,3,3,1,0,0,0,0,0,0,0},
        {0,0,0,0,1,3,3,1,0,0,0,0,0,0,0,0},
        {0,0,0,1,3,3,1,0,0,0,0,0,0,0,0,0},
        {0,0,1,4,4,1,0,0,0,0,0,0,0,0,0,0},
        {0,1,4,4,4,1,0,0,0,0,0,0,0,0,0,0},
        {1,4,4,4,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,4,4,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0},{0},
    },
    .pal={C(255,150,0),C(255,180,50),C(200,80,20),C(150,50,0),0,0,0}
};


  static const icon_t ICO_FEMBOYGL = {
    .px={
        {0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,2,2,2,1,0},
        {0,0,0,0,0,0,0,0,0,1,2,2,1,0,0,0},
        {0,0,0,0,0,0,0,0,1,2,2,1,0,0,0,0},
        {0,0,0,0,0,0,0,1,3,3,1,0,0,0,0,0},
        {0,0,0,0,0,0,1,3,3,1,0,0,0,0,0,0},
        {0,0,0,0,0,1,3,3,1,0,0,0,0,0,0,0},
        {0,0,0,0,1,3,3,1,0,0,0,0,0,0,0,0},
        {0,0,0,1,3,3,1,0,0,0,0,0,0,0,0,0},
        {0,0,1,4,4,1,0,0,0,0,0,0,0,0,0,0},
        {0,1,4,4,4,1,0,0,0,0,0,0,0,0,0,0},
        {1,4,4,4,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,4,4,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0},{0},
    },
    .pal={C(200,160,80),C(220,180,100),C(180,80,40),C(120,60,20),0,0,0}
};


static const icon_t *app_icon(int id){
    switch(id){
    case 0:  return &ICO_TERM;
    case 1:  return &ICO_FOLDER;
    case 2:  return &ICO_BROWSER;
    case 3:  return &ICO_SYSINFO;
    case 4:  return &ICO_SETTINGS;
    case 10: return &ICO_NANO;
    case 11: return &ICO_TOOLS;
    case 13: return &ICO_DRAW;
    case 30: return &ICO_TOOLS;
    case 32: return &ICO_NET;
    case 33: return &ICO_TOOLS;
    case 34: return &ICO_GAME;
    case 38: return &ICO_FEMBOYGL;
    case 36: return &ICO_TOOLS;
    case 45: return &ICO_GCC;
    case 40: case 41: case 42: case 43: case 44: case 46: case 47: return &ICO_NET;
    case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: return &ICO_SYSINFO;
    case 60: case 61: case 62: return &ICO_POWER;
    }
    if(id>=20&&id<30) return &ICO_GAME;
    if(id>=30&&id<40) return &ICO_TOOLS;
    return &ICO_TOOLS;
}




static const uint8_t CUR_PX[CUR_H][CUR_W]={
    {1,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},{1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},{1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},{1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},{1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,1,1,1,1,1,0,0},{1,2,2,1,2,2,1,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0},{1,1,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,0,1,2,1,0,0,0},{0,0,0,0,0,0,0,1,0,0,0,0},
    {0},{0},{0},{0}
};
static uint32_t _cur_save[CUR_W*CUR_H];
static int _cur_sx=-1,_cur_sy=-1;
static void cur_erase(void){
    if(_cur_sx<0)return;
    for(int r=0;r<CUR_H;r++){
        int py=_cur_sy+r;
        if(py<0||py>=vbe_screen_height)continue;
        uint32_t *row=vbe_bb_row(py);
        for(int c=0;c<CUR_W;c++){
            int px=_cur_sx+c;
            if(px<0||px>=vbe_screen_width)continue;
            row[px]=_cur_save[r*CUR_W+c];
        }
    }
    _cur_sx=-1;
}
static void cur_draw_hw(int cx,int cy){
    for(int r=0;r<CUR_H;r++){
        int py=cy+r;
        uint32_t *row=(py>=0&&py<vbe_screen_height)?vbe_bb_row(py):0;
        for(int c=0;c<CUR_W;c++){
            int px=cx+c;
            _cur_save[r*CUR_W+c]=(row&&px>=0&&px<vbe_screen_width)?row[px]:0;
        }
    }
    _cur_sx=cx;_cur_sy=cy;
    for(int r=0;r<CUR_H;r++){
        int py=cy+r;
        if(py<0||py>=vbe_screen_height)continue;
        uint32_t *row=vbe_bb_row(py);
        for(int c=0;c<CUR_W;c++){
            if(!CUR_PX[r][c])continue;
            int px=cx+c;
            if(px<0||px>=vbe_screen_width)continue;
            row[px]=(CUR_PX[r][c]==1)?C(0,180,220):C_WHITE;
        }
    }
}









typedef struct Window win_t;

static win_t g_wins[MAX_WINS];
static int   g_nwins=0,g_focus=-1;

static win_t *walloc(const char *t,int x,int y,int w,int h){
    if(g_nwins>=MAX_WINS)return 0;
    win_t *wi=&g_wins[g_nwins];
    *wi=(win_t){0};
    wi->alive=1;wi->x=x;wi->y=y;wi->w=w;wi->h=h;
    wi->resize_handle=-1;
    wi->dirty=1;
    wi->last_rect=(Rect){x,y,w,h};
    int i=0;while(t[i]&&i<47){wi->title[i]=t[i];i++;}wi->title[i]=0;
    g_focus=g_nwins;
    return &g_wins[g_nwins++];
}


static int win_resize_handle(win_t *wi,int px,int py){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    int g=RESIZE_GRIP;

    int inL=(px>=x       && px<x+g);
    int inR=(px>=x+w-g   && px<x+w);
    int inT=(py>=y       && py<y+g);
    int inB=(py>=y+h-g   && py<y+h);
    int inX=(px>=x && px<x+w);
    int inY=(py>=y && py<y+h);
    if(inT&&inL) return 0;
    if(inT&&inR) return 2;
    if(inB&&inL) return 5;
    if(inB&&inR) return 7;
    if(inT&&inX) return 1;
    if(inB&&inX) return 6;
    if(inL&&inY) return 3;
    if(inR&&inY) return 4;
    return -1;
}

static void wwrite(win_t *wi,const char *s){
    while(*s){
        int pos=(wi->bstart+wi->blen)%TBSZ;
        wi->buf[pos]=*s++;
        if(wi->blen<TBSZ)wi->blen++;
        else wi->bstart=(wi->bstart+1)%TBSZ;
    }
}

static void twin_clear(win_t *wi){wi->bstart=0;wi->blen=0;}

typedef struct {
    win_t *wi;
    char  cmd[INPUT_MAX+2];
} ap_task_t;

static void drawwin(win_t *wi,int foc);
static win_t *g_tterm=0;
static uint32_t g_thook_last=0;
static void thook(char c){
    if(!g_tterm)return;
    char b[2];b[0]=c;b[1]=0;wwrite(g_tterm,b);
    uint32_t now=_ticks();
    if((uint32_t)(now-g_thook_last)<32)return;
    g_thook_last=now;
    drawwin(g_tterm,1);
    vbe_flip_rect(g_tterm->x,g_tterm->y,g_tterm->w,g_tterm->h);
}
static void gui_clear_hook(void){if(g_tterm)twin_clear(g_tterm);}

static void _exec_inline(win_t *wi, const char *cmd){
    win_t *prev=g_tterm;
    g_tterm=wi;
    vbe_set_autoflip(0);
    terminal_set_hook(thook);
    terminal_set_clear_hook(gui_clear_hook);
    shell_exec_cmd(cmd);
    terminal_set_hook(0);
    terminal_set_clear_hook(0);
    vbe_set_autoflip(0);
    g_tterm=prev;
}

static ap_task_t g_ap_task;
static void _ap_exec_worker(void *arg){
    ap_task_t *t=(ap_task_t*)arg;
    _exec_inline(t->wi, t->cmd);
    t->wi->busy=0;
}

static void run_cmd(win_t *wi, const char *cmd){
    extern cpu_t g_cpus[];
    extern int   g_cpu_count;
    int ap_ok=0;
    for(int i=0;i<g_cpu_count;i++){
        if(!g_cpus[i].is_bsp&&g_cpus[i].online){ap_ok=1;break;}
    }
    if(ap_ok && !wi->busy){
        wi->busy=1;
        _scpy(g_ap_task.cmd,cmd,INPUT_MAX+1);
        g_ap_task.wi=wi;
        smp_dispatch_ap(_ap_exec_worker,&g_ap_task,0);
    } else {
        _exec_inline(wi, cmd);
    }
}

static void smact_run(win_t *w, const char *cmd){
    if(!w||!cmd)return;
    wwrite(w,"$ ");wwrite(w,cmd);wwrite(w,"\n");
    _scpy(w->inp,cmd,INPUT_MAX+1);
    w->inlen=_slen(cmd);
    run_cmd(w,cmd);
    w->inlen=0;
}

static void fm_populate(win_t *wi){
    wi->fm_count=0;
    sfs_node_t *nd=sfs_resolve(sfs_root(),wi->fm_path[0]?wi->fm_path:"/");
    if(!nd||nd->type!=SFS_DIR)return;
    if(!(wi->fm_path[0]=='/'&&wi->fm_path[1]==0)){
        const char *dd="..";int i=0;
        while(dd[i]&&i<FM_NAMELEN-1){wi->fm_entries[wi->fm_count].name[i]=dd[i]; i++;}
        wi->fm_entries[wi->fm_count].name[i]=0;
        wi->fm_entries[wi->fm_count].is_dir=1;
        wi->fm_count++;
    }
    for(sfs_node_t *c=nd->child;c&&wi->fm_count<FM_MAXENTRIES;c=c->next){
        int i=0;while(c->name[i]&&i<FM_NAMELEN-1){wi->fm_entries[wi->fm_count].name[i]=c->name[i];i++;}
        wi->fm_entries[wi->fm_count].name[i]=0;
        wi->fm_entries[wi->fm_count].is_dir=(c->type==SFS_DIR);
        wi->fm_count++;
    }
}

static void fm_navigate(win_t *wi,int idx){
    if(idx<0||idx>=wi->fm_count)return;
    fm_entry_t *e=&wi->fm_entries[idx];
    if(!e->is_dir)return;
    if(e->name[0]=='.'&&e->name[1]=='.'&&e->name[2]==0){
        int len=0;while(wi->fm_path[len])len++;
        while(len>1&&wi->fm_path[len-1]!='/')len--;
        if(len>1)len--;
        wi->fm_path[len]=0;
        if(!wi->fm_path[0]){wi->fm_path[0]='/';wi->fm_path[1]=0;}
    } else {
        int plen=0;while(wi->fm_path[plen])plen++;
        if(plen==0||wi->fm_path[plen-1]!='/'){wi->fm_path[plen]='/';plen++;}
        int i=0;while(e->name[i]&&plen+i<254){wi->fm_path[plen+i]=e->name[i];i++;}
        wi->fm_path[plen+i]=0;
    }
    wi->fm_sel=0;wi->fm_scroll=0;fm_populate(wi);
}

static void br_fetch(win_t *wi){
    wi->br_loading=1;wi->br_len=0;
    const char *url=wi->br_url;
    if(url[0]=='h'&&url[1]=='t'&&url[2]=='t'&&url[3]=='p'&&url[4]==':'&&url[5]=='/'&&url[6]=='/')url+=7;
    char host[128];char path[256];
    int hi=0;path[0]='/';path[1]=0;
    while(*url&&*url!='/'&&hi<127) {
        host[hi++] = *url++;
    }
    host[hi] = 0;
    if(*url=='/'){
        int pi=0;
        while(*url&&pi<254) {
            path[pi++]=*url++;
        }
        path[pi]=0;
    }
    uint8_t *body=0;size_t blen=0;
    int r=http_get(host,path,&body,&blen);
    if(r!=0||!body||blen==0){
        const char *err="[browser] could not fetch page.\n1pwease check network with 'dhcp'.\n";
        int i=0;while(err[i]&&i<BR_BUFSZ-1){wi->br_buf[i]=err[i];i++;}wi->br_len=i;
        wi->br_loading=0;if(body)kfree(body);return;
    }
    int out=0,in_tag=0;
    for(size_t i=0;i<blen&&out<BR_BUFSZ-2;i++){
        char c=(char)body[i];
        if(c=='<'){in_tag=1;continue;}
        if(c=='>'){in_tag=0;continue;}
        if(in_tag||c=='\r')continue;
        wi->br_buf[out++]=c;
    }
    wi->br_buf[out]=0;wi->br_len=out;
    kfree(body);wi->br_loading=0;wi->br_scroll=0;
}


static void draw_titlebar(win_t *wi,int foc,const icon_t *ico,const char *title,int closebtn_only){
    int x=wi->x,y=wi->y,w=wi->w;
    uint32_t tc=foc?C_WTA:C_WTI;
    dr(x,y,w,TITLE_H,tc);
    if(ico) dricon(x+8,y+(TITLE_H-16)/2,ico);
    ds(x+30,y+(TITLE_H-FH)/2,title,foc?C_WHITE:C_MGREY,tc);
    int bbase=x+w-WBTN_W*3;
    if(!closebtn_only){
        dr(bbase,y,WBTN_W,TITLE_H,foc?C_MINHOV:tc);
        dr(bbase+(WBTN_W-FW)/2,y+TITLE_H-8,FW,2,foc?C_WHITE:C_MGREY);
        dr(bbase+WBTN_W,y,WBTN_W,TITLE_H,foc?C_MINHOV:tc);
        dbdr(bbase+WBTN_W+(WBTN_W-10)/2,y+(TITLE_H-10)/2,10,10,foc?C_WHITE:C_MGREY);
    }
    dr(bbase+WBTN_W*2,y,WBTN_W,TITLE_H,foc?C_CLOSE:tc);
    ds(bbase+WBTN_W*2+(WBTN_W-FW)/2,y+(TITLE_H-FH)/2,"X",C_WHITE,foc?C_CLOSE:tc);
    if(foc)dr(x,y+TITLE_H-2,w-WBTN_W*3,2,C_ACCENT);
}

static void draw_textbuf(win_t *wi,int bx,int by,int bw,int bh,uint32_t fg,uint32_t bg){
    int cols=(bw-4)/FW; if(cols<1)cols=1;
    int rows=bh/FH; if(rows<1)rows=1;
    dr(bx,by,bw,bh,bg);
    int nlines=0;
    static int lidx[MAX_LINESZ],llen[MAX_LINESZ];
    lidx[0]=0;llen[0]=0;
    for(int i=0;i<wi->blen&&nlines<MAX_LINESZ-1;i++){
        char c=wi->buf[(wi->bstart+i)%TBSZ];
        if(c=='\n'){nlines++;lidx[nlines]=i+1;llen[nlines]=0;}
        else llen[nlines]++;
    }
    if(llen[nlines]>0)nlines++;
    int first=nlines-rows-wi->scroll; if(first<0)first=0;
    for(int ln=0;ln<rows;ln++){
        int li=first+ln; if(li>=nlines)break;
        int ll=llen[li]; if(ll>cols)ll=cols;
        int ry=by+ln*FH;
        for(int ci=0;ci<ll;ci++){
            char ch=wi->buf[(wi->bstart+lidx[li]+ci)%TBSZ];
            if(ch > 32) vbe_draw_char_mask(bx+2+ci*FW,ry,ch,fg);
        }
    }
}

static int _is_py_kw(const char *s, int len) {
    const char *kws[] = {"def","class","if","else","elif","for","while","import","from","as","return","pass","in","is","not","and","or","lambda","with","try","except","None","True","False"};
    for(int i=0; i<24; i++) {
        int match=1; for(int j=0; j<len; j++) if(kws[i][j]!=s[j]) {match=0;break;}
        if(match && kws[i][len]==0) return 1;
    }
    return 0;
}

static int _is_c_kw(const char *s, int len) {
    const char *kws[] = {"int","char","float","double","void","if","else","for","while","do","return","switch","case","break","continue","static","extern","struct","typedef","union","enum","const","unsigned","signed","include","define"};
    for(int i=0; i<26; i++) {
        int match=1; for(int j=0; j<len; j++) if(kws[i][j]!=s[j]) {match=0;break;}
        if(match && kws[i][len]==0) return 1;
    }
    return 0;
}

static void draw_nano_buffer(win_t *wi, int bx, int by, int bw, int bh, int lnw) {
    int cols = (bw-lnw-10)/FW; if(cols<1) cols=1;
    int rows = bh/FH; if(rows<1) rows=1;

    int nlines=0;
    static int lidx[MAX_LINESZ], llen[MAX_LINESZ];
    lidx[0]=0; llen[0]=0;
    for(int i=0; i<wi->blen && nlines<MAX_LINESZ-1; i++) {
        char c=wi->buf[(wi->bstart+i)%TBSZ];
        if(c=='\n') { nlines++; lidx[nlines]=i+1; llen[nlines]=0; }
        else llen[nlines]++;
    }
    if(llen[nlines]>0) nlines++;

    int first = wi->scroll;
    for(int ln=0; ln<rows; ln++) {
        int li = first + ln; if(li >= nlines && li > 0) break;
        int ry = by + ln*FH;


        char lnb[8]; _itoa(lnb, li+1);
        ds(bx+2, ry, lnb, C(80,80,120), C(24,24,32));

        if(li >= nlines) continue;


        int ll = llen[li]; if(ll > cols) ll = cols;
        int tx = bx + lnw + 6;
        int in_str = 0;
        for(int ci=0; ci<ll; ci++) {
            char ch = wi->buf[(wi->bstart + lidx[li] + ci)%TBSZ];
            uint32_t color = C(220,220,180);


            if(ch=='\'' || ch=='\"') in_str = !in_str;
            if(in_str) color = C(150,220,150);
            else if(ch=='

                for(int k=ci; k<ll; k++) vbe_draw_char_mask(tx+(k*FW), ry, wi->buf[(wi->bstart+lidx[li]+k)%TBSZ], C(100,100,100));
                break;
            } else if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')) {
                int kw_len=0;
                while(ci+kw_len < ll) {
                    char kt = wi->buf[(wi->bstart+lidx[li]+ci+kw_len)%TBSZ];
                    if(!((kt>='a'&&kt<='z')||(kt>='A'&&kt<='Z')||(kt>='0'&&kt<='9')||(kt=='_'))) break;
                    kw_len++;
                }
                char kw_buf[32];
                int cp_len = kw_len<31?kw_len:31;
                for(int k=0; k<cp_len; k++) kw_buf[k]=wi->buf[(wi->bstart+lidx[li]+ci+k)%TBSZ];
                kw_buf[cp_len]=0;

                if(_is_py_kw(kw_buf, cp_len) || _is_c_kw(kw_buf, cp_len)) color = C(160,80,255);
                else if(kw_buf[0]>='0' && kw_buf[0]<='9') color = C(255,180,80);

                for(int k=0; k<kw_len; k++) {
                   vbe_draw_char_mask(tx+(ci*FW), ry, wi->buf[(wi->bstart+lidx[li]+ci)%TBSZ], color);
                   ci++;
                }
                ci--; continue;
            }

            if(ch > 32) vbe_draw_char_mask(tx+(ci*FW), ry, ch, color);
        }
    }
}


static void draw_inputbar(win_t *wi,int ix,int iy,int iw,int foc,const char *prompt,uint32_t pc){
    int ih=FH+8;
    dr(ix,iy,iw,ih,C_TINP);
    dr(ix,iy,3,ih,C_ACCENT);
    int px=ix+8;
    ds(px,iy+4,prompt,pc,C_TINP); px+=_slen(prompt)*FW;
    dsn(px,iy+4,wi->inp,wi->inlen,C_WHITE,C_TINP); px+=wi->inlen*FW;
    if(foc&&(_ticks()/500)&1) vbe_draw_char(px,iy+4,'_',C_WHITE,C_TINP);
}


static file_picker_t *fp_create(int mode, const char *title){
    (void)title;
    file_picker_t *fp=(file_picker_t*)kmalloc(sizeof(file_picker_t));
    if(!fp)return 0;
    memset(fp,0,sizeof(file_picker_t));
    fp->active=1;
    fp->mode=mode;
    fp->path[0]='/';
    fp->path[1]=0;
    return fp;
}

static void fp_populate(file_picker_t *fp, int app_type){
    if(!fp)return;
    fp->entry_count=0;
    sfs_node_t *nd=sfs_resolve(sfs_root(),fp->path[0]?fp->path:"/");
    if(!nd||nd->type!=SFS_DIR)return;
    if(!(fp->path[0]=='/'&&fp->path[1]==0)){
        _scpy(fp->entries[fp->entry_count],"..",FM_NAMELEN);
        fp->is_dir[fp->entry_count]=1;
        fp->entry_count++;
    }
    for(sfs_node_t *c=nd->child;c&&fp->entry_count<FM_MAXENTRIES;c=c->next){
        int is_dir = (c->type==SFS_DIR);


        if(!is_dir && app_type == WAPP_MEDIAPLAYER) {
            int len = _slen(c->name);
            int match = 0;
            if(len > 4) {
                const char *s = c->name + len - 4;
                if((s[0]=='.' && s[1]=='s' && s[2]=='a' && s[3]=='f') ||
                   (s[0]=='.' && s[1]=='s' && s[2]=='v' && s[3]=='f')) match = 1;
            }
            if(!match) continue;
        }

        int i=0;while(c->name[i]&&i<FM_NAMELEN-1){fp->entries[fp->entry_count][i]=c->name[i];i++;}
        fp->entries[fp->entry_count][i]=0;
        fp->is_dir[fp->entry_count]=is_dir;
        fp->entry_count++;
    }
}

static void fp_navigate(file_picker_t *fp, int idx, int app_type){
    if(!fp||idx<0||idx>=fp->entry_count)return;
    if(!fp->is_dir[idx])return;
    if(fp->entries[idx][0]=='.'&&fp->entries[idx][1]=='.'&&fp->entries[idx][2]==0){
        int len=0;while(fp->path[len])len++;
        while(len>1&&fp->path[len-1]!='/')len--;
        if(len>1)len--;
        fp->path[len]=0;
        if(!fp->path[0]){fp->path[0]='/';fp->path[1]=0;}
    } else {
        int plen=0;while(fp->path[plen])plen++;
        if(plen==0||fp->path[plen-1]!='/'){fp->path[plen]='/';plen++;}
        int i=0;while(fp->entries[idx][i]&&plen+i<254){fp->path[plen+i]=fp->entries[idx][i];i++;}
        fp->path[plen+i]=0;
    }
    fp->sel=0;fp->scroll=0;fp_populate(fp, app_type);
}

static void fp_draw(file_picker_t *fp, int foc){
    if(!fp||!fp->active)return;
    int sw=vbe_screen_width,sh=vbe_screen_height;
    int fx=60,fy=40,fw=sw-120,fh=sh-80;


    dr(fx+4,fy+4,fw,fh,C_BLACK);
    dr(fx,fy,fw,fh,C(32,32,32));
    dbdr(fx,fy,fw,fh,foc?C_ACCENT:C(60,60,60));


    dr(fx,fy,fw,32,foc?C(20,20,20):C(30,30,30));
    ds(fx+10,fy+8,fp->mode==FP_MODE_OPEN?"Open":"Save As",C_WHITE,foc?C(20,20,20):C(30,30,30));


    int bbase=fx+fw-46;
    int hov_close = pin(mouse_x,mouse_y,bbase,fy,46,32);
    if(hov_close) dr(bbase,fy,46,32,C_RED);
    ds(bbase+19,fy+8,"X",C_WHITE,hov_close?C_RED:(foc?C(20,20,20):C(30,30,30)));


    int aby=fy+32;
    dr(fx,aby,fw,38,C(45,45,45));
    dbdr(fx+5,aby+5,fw-10,28,C(80,80,80));
    ds(fx+15,aby+10,fp->path[0]?fp->path:"/",C_WHITE,C(45,45,45));


    int sdx=fx, sdy=aby+38, sdw=150, sdh=fh-32-38-60;
    dr(sdx,sdy,sdw,sdh,C(32,32,32));
    dr(sdx+sdw-1,sdy,1,sdh,C(60,60,60));

    const char *places[] = {"Quick access", "This PC", "Desktop", "Documents", "Downloads"};
    for(int i=0; i<5; i++) {
        int py = sdy + 10 + i*28;
        int hov = pin(mouse_x,mouse_y,sdx,py,sdw,28);
        if(hov) dr(sdx,py,sdw,28,C(50,50,50));
        ds(sdx+15, py+6, places[i], C_LGREY, hov?C(50,50,50):C(32,32,32));
    }


    int ldx=sdx+sdw, ldy=sdy, ldw=fw-sdw, ldh=sdh;
    dr(ldx,ldy,ldw,ldh,C(25,25,25));

    int row_h=24, vis=ldh/row_h;
    for(int i=0;i<vis;i++){
        int fi=i+fp->scroll;
        if(fi>=fp->entry_count)break;
        int ry=ldy+i*row_h;
        int hov = pin(mouse_x,mouse_y,ldx,ry,ldw,row_h);
        uint32_t bg=(fi==fp->sel)?C_ACCENT:(hov?C(50,50,50):C(25,25,25));
        dr(ldx,ry,ldw,row_h,bg);
        dricon(ldx+8,ry+(row_h-16)/2,fp->is_dir[fi]?&ICO_FOLDER:&ICO_NANO);
        ds(ldx+32,ry+(row_h-FH)/2,fp->entries[fi],(fi==fp->sel||hov)?C_WHITE:C_LGREY,bg);
    }


    int fty=fy+fh-60;
    dr(fx,fty,fw,60,C(32,32,32));
    dr(fx,fty,fw,1,C(60,60,60));

    ds(fx+sdw+10, fty+15, "File name:", C_WHITE, C(32,32,32));
    int tbx=fx+sdw+90, tbw=fw-sdw-90-180;
    dr(tbx,fty+12,tbw,26,C(20,20,20));
    dbdr(tbx,fty+12,tbw,26,C(100,100,100));
    dsn(tbx+5,fty+17,fp->input,fp->input_len,C_WHITE,C(20,20,20));
    if(foc&&(_ticks()/500)&1) vbe_draw_char(tbx+5+fp->input_len*FW,fty+17,'_',C_WHITE,C(20,20,20));


    int btnw=80, btnh=28;
    int obx=fx+fw-170, oby=fty+12;
    int hov_open = pin(mouse_x,mouse_y,obx,oby,btnw,btnh);
    uint32_t obgc = hov_open?C(0,140,240):C_ACCENT;
    dr(obx,oby,btnw,btnh,obgc);
    const char *btxt = fp->mode==FP_MODE_OPEN?"Open":"Save";
    ds(obx+(btnw-_slen(btxt)*FW)/2, oby+8, btxt, C_WHITE, obgc);


    int cbx=fx+fw-85, cby=fty+12;
    int hov_can = pin(mouse_x,mouse_y,cbx,cby,btnw,btnh);
    uint32_t cbgc = hov_can?C(60,60,60):C(45,45,45);
    dr(cbx,cby,btnw,btnh,cbgc);
    dbdr(cbx,cby,btnw,btnh,C(100,100,100));
    ds(cbx+(btnw-6*FW)/2, cby+8, "Cancel", C_WHITE, cbgc);
}


static void drawfm(win_t *wi, int foc) {
    int x=wi->x, y=wi->y, w=wi->w, h=wi->h;
    dr(x+4, y+4, w, h, C_BLACK);
    draw_titlebar(wi, foc, &ICO_FOLDER, "stinger Explorer", 1);

    int sw=130;
    dr(x, y+TITLE_H, sw, h-TITLE_H, C(22,22,28));
    dr(x+sw-1, y+TITLE_H, 1, h-TITLE_H, C(40,40,50));

    const char *tabs[]={"Filesystem", "Storage", "Network", "Journal
    for(int i=0; i<4; i++){
        int ty=y+TITLE_H+8+i*28;
        if(i==wi->fm_tab) dr(x+4, ty, sw-8, 24, C_SM_SEL);
        ds(x+10, ty+5, tabs[i], C_WHITE, i==wi->fm_tab?C_SM_SEL:C(22,22,28));
    }


    if(wi->fm_tab == 3) {
        ds(x+sw/2-16, y+h-40, "/\\", C_YELLOW, C(22,22,28));
        ds(x+sw/2-24, y+h-24, "/ o \\", C_YELLOW, C(22,22,28));
    }

    int cx=x+sw, cw=w-sw, cy=y+TITLE_H;
    dr(cx, cy, cw, h-TITLE_H, C_WBG);

    int row_h=24;
    for(int i=0; i<(h-TITLE_H)/row_h && i+wi->fm_scroll < wi->fm_count; i++){
        int fidx=i+wi->fm_scroll;
        int fy=cy+i*row_h;
        uint32_t bg=(fidx==wi->fm_sel)?C_SM_SEL:(i%2?C(24,24,30):C_WBG);
        dr(cx, fy, cw, row_h, bg);
        dricon(cx+8, fy+(row_h-16)/2, wi->fm_entries[fidx].is_dir?&ICO_FOLDER:&ICO_NANO);
        ds(cx+30, fy+(row_h-FH)/2, wi->fm_entries[fidx].name, C_WHITE, bg);
    }
    dbdr(x, y, w, h, foc?C_WBDR_A:C_WBDR_I);
}


static void drawbr(win_t *wi, int foc) {
    int x=wi->x, y=wi->y, w=wi->w, h=wi->h;
    dr(x+4, y+4, w, h, C_BLACK);
    draw_titlebar(wi, foc, &ICO_NET, "stinger Navigator", 1);

    int aby=y+TITLE_H, abh=34;
    dr(x, aby, w, abh, C(22,22,28));
    dr(x+8, aby+6, w-80, 22, C(12,12,16));
    dbdr(x+8, aby+6, w-80, 22, foc?C_SM_SEL:C(40,40,50));
    ds(x+15, aby+8, wi->br_url[0]?wi->br_url:"http://yaoi.net", C_WHITE, C(12,12,16));

    uint32_t btn_c = foc ? C_SM_SEL : C(60,60,70);
    dr(x+w-65, aby+6, 56, 22, btn_c);
    ds(x+w-58, aby+8, "FIND", C_WHITE, btn_c);

    int cy=aby+abh, ch=h-TITLE_H-abh;
    dr(x, cy, w, ch<0?0:ch, C(245,245,250));
    if(wi->br_loading) {
        ds(x+w/2-30, cy+ch/2, "Loading...", C(80,80,90), C(245,245,250));
    } else {
        draw_textbuf(wi, x+12, cy+12, w-24, ch-24, C(20,20,30), C(245,245,250));
    }
    dbdr(x, y, w, h, foc?C_WBDR_A:C_WBDR_I);
}



static void draw_srow(int rx,int ry,int rw,int rh,uint32_t bg,
                      const char *label,const char *val,uint32_t vc){
    dr(rx,ry,rw,rh,bg);
    ds(rx+10,ry+(rh-FH)/2,label,C_MGREY,bg);
    if(val)ds(rx+172,ry+(rh-FH)/2,val,vc,bg);
}

static void draw_stoggle(int rx,int ry,int on){
    uint32_t trk=on?C(0,100,50):C(42,42,42);
    dr(rx,ry,36,16,trk); dbdr(rx,ry,36,16,on?C(0,180,90):C(62,62,62));
    dr(on?rx+22:rx+2,ry+2,12,12,on?C(0,240,110):C(105,105,105));
}
static void drawsettings(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C(5,5,8));
    draw_titlebar(wi,foc,&ICO_SETTINGS,"Settings",1);

    int sbw=164;
    dr(x,y+TITLE_H,sbw,h-TITLE_H,C(16,16,24));
    dr(x+sbw-1,y+TITLE_H,1,h-TITLE_H,C(32,32,48));
    dr(x,y+TITLE_H,sbw,30,C(10,10,18));
    ds(x+10,y+TITLE_H+7,"SETTINGS",C(60,60,95),C(10,10,18));
    const char *stabs[]={"  Network","  Display","  System","  Audio",
                         "  Theme","  Storage","  Users","  About"};
    const icon_t *sicos[]={&ICO_NET,&ICO_SYSINFO,&ICO_SYSINFO,&ICO_TOOLS,
                           &ICO_DRAW,&ICO_FOLDER,&ICO_SETTINGS,&ICO_SCORP};
    int ntabs=8,tab_h=36;
    for(int i=0;i<ntabs;i++){
        int ty=y+TITLE_H+30+i*tab_h;
        if(ty+tab_h>y+h-4)break;
        int sel=(i==wi->settings_tab);
        uint32_t bg2=sel?C(0,72,144):C(16,16,24);
        dr(x,ty,sbw,tab_h,bg2);
        if(sel)dr(x,ty,3,tab_h,C(0,180,255));
        dr(x,ty+tab_h-1,sbw,1,C(26,26,40));
        dricon(x+10,ty+(tab_h-16)/2,sicos[i]);
        ds(x+32,ty+(tab_h-FH)/2,stabs[i],sel?C_WHITE:C(95,95,130),bg2);
    }

    int cx2=x+sbw,cy2=y+TITLE_H,cw2=w-sbw;
    dr(cx2,cy2,cw2,h-TITLE_H,C(12,12,20));
    dr(cx2,cy2,cw2,36,C(16,16,28));
    dr(cx2,cy2+35,cw2,1,C(32,32,52));
    if(wi->settings_tab<ntabs){
        dricon(cx2+8,cy2+(36-16)/2,sicos[wi->settings_tab]);
        ds(cx2+30,cy2+(36-FH)/2,stabs[wi->settings_tab],C_WHITE,C(16,16,28));
    }
    int ry=cy2+46,lx=cx2+8,rw=cw2-16,row_h=34;
    uint32_t r0=C(18,18,30),r1=C(15,15,25);
    wi->settings_tab_extra=0;
    switch(wi->settings_tab){

    case 0:{
        net_config_t cfg; net_get_config(&cfg);
        char buf[64];


        int p=0;{uint8_t*a=cfg.ip;for(int n=0;n<4;n++){uint8_t v=a[n];char tmp[4];int ti=0;
            if(!v){tmp[ti++]='0';}else{while(v){tmp[ti++]='0'+v%10;v/=10;}}
            while(ti>0){buf[p++]=tmp[--ti];} if(n<3)buf[p++]='.';}buf[p]=0;}
        draw_srow(lx,ry,rw,row_h,r0,"IP Address",buf,C_WHITE); ry+=row_h+2;


        char wifi_status_buf[64];
        wifi_status(wifi_status_buf, 64);
        draw_srow(lx,ry,rw,row_h,r1,"WiFi Status",wifi_status_buf,C_CYAN); ry+=row_h+2;


        int bw2=112, bh2=28;
        int hov_scan = pin(mouse_x, mouse_y, lx, ry, bw2, bh2);
        uint32_t scan_bg = hov_scan ? C(0, 100, 50) : C(0, 85, 42);
        dr(lx, ry, bw2, bh2, scan_bg);
        dbdr(lx, ry, bw2, bh2, C(0, 170, 85));
        ds(lx+12, ry+(bh2-FH)/2, "Scan WiFi", C_WHITE, scan_bg);

        if (hov_scan && (mouse_buttons & 1)) {
            wifi_scan();
        }
        ry += row_h + 10;


        ds(lx+8, ry, "Available Networks:", C_MGREY, C(12, 12, 20)); ry += 20;
        for(int i=0; i<wifi_ap_count && i<5; i++) {
            char ap_line[64];
            _scpy(ap_line, wifi_aps[i].ssid, 32);
            draw_srow(lx, ry, rw, row_h, (i%2?r1:r0), ap_line, NULL, C_WHITE);
            ry += row_h + 2;
        }
        break;}

    case 1:{
        extern int vbe_screen_width,vbe_screen_height;
        int wv=vbe_screen_width,hv=vbe_screen_height;
        char wb2[8],hb2[8],res2[20];
        {int t=0;char tmp[8];int ti=0;while(wv){tmp[ti++]='0'+wv%10;wv/=10;}
         if(!ti) tmp[ti++]='0';
         while(ti>0) wb2[t++]=tmp[--ti];
         wb2[t]=0;
    }
        {int t=0;char tmp[8];int ti=0;while(hv){tmp[ti++]='0'+hv%10;hv/=10;}
         if(!ti) tmp[ti++]='0';
         while(ti>0) hb2[t++]=tmp[--ti];
         hb2[t]=0;
    }
        int ri2=0;for(int i=0;wb2[i];i++)res2[ri2++]=wb2[i];
        res2[ri2++]='x';for(int i=0;hb2[i];i++)res2[ri2++]=hb2[i];res2[ri2]=0;
        draw_srow(lx,ry,rw,row_h,r0,"Driver","VBE VESA Framebuffer",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Resolution",res2,C(120,200,255));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Colour Depth","32 bpp  (BGRA)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Back Buffer","Enabled (double-buffered)",C(0,220,80));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Compositor","YaoiGUI v4.0  software",C_WHITE);
        break;}

    case 2:{
        extern int g_cpu_count;
        char cb2[4];cb2[0]='0'+(char)(g_cpu_count<9?g_cpu_count:9);cb2[1]=0;
        draw_srow(lx,ry,rw,row_h,r0,"OS Version","stinger v4.0",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Architecture","x86  32-bit  (i686)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"CPU Cores",cb2,C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Kernel","Monolithic  in-process",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Scheduler","Round-robin  SMP",C_WHITE);ry+=row_h+8;
        sfs_node_t*_etc2=sfs_resolve(sfs_root(),"/etc");
        int gui_on=1;
        if(_etc2){sfs_node_t*_f2=sfs_find_child(_etc2,"gui_autostart");
                  if(_f2&&_f2->data&&_f2->size>0)gui_on=(((char*)_f2->data)[0]!='0');}
        dr(lx,ry,rw,row_h,C(20,20,34));
        ds(lx+10,ry+(row_h-FH)/2,"GUI Autostart on Boot",C_MGREY,C(20,20,34));
        draw_stoggle(lx+rw-56,ry+(row_h-16)/2,gui_on);
        ds(lx+rw-12,ry+(row_h-FH)/2,gui_on?"ON ":"OFF",
           gui_on?C(0,220,80):C(100,100,100),C(20,20,34));
        wi->settings_tab_extra=2*10000+ry;
        break;}

    case 3:{
        draw_srow(lx,ry,rw,row_h,r0,"Sound Device","PC Speaker  (8253/8254)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Output","Beep / square wave",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"MIDI","Unavailable  (no UART MIDI)",C(90,90,90));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"HDMI Audio","Not implemented",C(90,90,90));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Status","Active",C(0,220,80));ry+=row_h+10;
        ds(lx+4,ry,">  Run 'beep' in terminal to test audio output.",C(50,50,80),C(12,12,20));
        break;}

    case 4:{
        draw_srow(lx,ry,rw,row_h,r0,"Colour Mode","Dark  (system default)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Font","IBM VGA 8x16  (baked in)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Window Style","Flat glass  dark chrome",C_WHITE);ry+=row_h+8;

        ds(lx+8, ry, "Wallpaper Selector", C_WHITE, C(12,12,20)); ry += 24;


        sfs_node_t *wp_dir = sfs_resolve(sfs_root(), "/wallpapers");
        if(wp_dir && wp_dir->type == SFS_DIR) {
            sfs_node_t *f = wp_dir->child;
            for(int i=0; f && i < 8; i++) {
                if(f->type == SFS_FILE) {

                    int hov = pin(mouse_x, mouse_y, lx, ry, rw, row_h);
                    uint32_t bg = hov ? C(60,60,80) : (i%2?r1:r0);
                    dr(lx, ry, rw, row_h, bg);
                    ds(lx+10, ry+(row_h-FH)/2, f->name, C_WHITE, bg);

                    if (hov && (mouse_buttons & 1)) {
                        gui_load_wallpaper(f->name);
                    }
                    ry += row_h + 2;
                }
                f = f->next;
            }
        } else {
            ds(lx+10, ry, "No wallpapers in /wallpapers", C_RED, C(12,12,20));
        }
        break;}

    case 5:{
        sfs_node_t*root3=sfs_root();
        draw_srow(lx,ry,rw,row_h,r0,"Filesystem","SFS  (stinger FS)  in-memory",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Mount Point","/  (ramfs  volatile)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Persistence","Flushed to ATA on writes",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Root Node",root3?"Mounted":"Error",
                  root3?C(0,220,80):C(220,80,80));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Block Driver","ATA PIO  28-bit LBA",C_WHITE);ry+=row_h+10;
        ds(lx+4,ry,">  Use 'diskinfo' or 'df' in terminal for live stats.",C(50,50,80),C(12,12,20));
        break;}

    case 6:{
        const char*uname2=current_user.username[0]?current_user.username:"root";
        const char*uhome2=current_user.home[0]?current_user.home:"/root";
        draw_srow(lx,ry,rw,row_h,r0,"Username",uname2,C(120,200,255));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Home Directory",uhome2,C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Shell","/bin/sh  (built-in stinger)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Auth","Password  (SHA-256)",C_WHITE);ry+=row_h+10;
        ds(lx+4,ry,">  Use 'passwd' in terminal to change your password.",C(50,50,80),C(12,12,20));
        break;}

    case 7:{
        dr(lx,ry,rw,52,C(10,10,22));dbdr(lx,ry,rw,52,C(28,28,52));
        dricon(lx+10,ry+18,&ICO_SCORP);
        ds(lx+34,ry+10,"stinger",C_WHITE,C(10,10,22));
        ds(lx+34,ry+26,"v4.0  YaoiGUI Edition",C(0,160,80),C(10,10,22));
        ry+=62;
        draw_srow(lx,ry,rw,row_h,r0,"Build","indev8  (custom monolithic)",C(120,160,255));ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"Bootloader","GRUB 2  (Multiboot 1)",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Toolchain","i686-linux-gnu-gcc  -O2",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r1,"License","MIT License",C_WHITE);ry+=row_h+2;
        draw_srow(lx,ry,rw,row_h,r0,"Target","x86 real hardware + QEMU",C_WHITE);
        break;}
    }
    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawcalc(win_t *wi, int foc) {
    int x=wi->x, y=wi->y, w=wi->w, h=wi->h;
    dr(x+4, y+4, w, h, C_BLACK);
    draw_titlebar(wi, foc, &ICO_TOOLS, "Crytal Castles Calc", 1);
    int dy=y+TITLE_H+8, dh=44;
    dr(x+8, dy, w-16, dh, C(12,12,18));
    dbdr(x+8, dy, w-16, dh, foc?C_ACCENT:C(50,50,60));

    if(wi->inlen>0) ds(x+14, dy+14, wi->inp, C_WHITE, C(12,12,18));
    else ds(x+w-24, dy+14, "0", C_WHITE, C(12,12,18));
    int bx=x+8, by=dy+dh+10, bw=(w-26)/4, bh=(h-TITLE_H-dh-40)/5;
    const char *btns_new[]={"7","8","9","/", "4","5","6","*", "1","2","3","-", "0",".","=","+", "C","(",")","%"};
    for(int i=0; i<20; i++){
        int r=i/4, c=i%4;
        int px=bx+c*(bw+3);
        int py=by+r*(bh+3);
        uint32_t bc = (i==16)?C(160,40,40): (i==14)?C_ACCENT : C(32,32,44);
        dr(px, py, bw, bh, bc);
        dbdr(px, py, bw, bh, foc?C(60,60,95):C(40,40,55));
        ds(px+bw/2-4, py+bh/2-8, btns_new[i], C_WHITE, bc);
    }
    dbdr(x, y, w, h, foc?C_WBDR_A:C_WBDR_I);
}


static void drawhexcalc(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_TOOLS,"Hex Calculator",1);
    int cy=y+TITLE_H+8;
    int inph=FH+8;
    dr(x,y+TITLE_H,w,h-TITLE_H,C_WBG);
    int ca_h=h-TITLE_H-inph-20;
    dr(x+8,cy,w-16,ca_h,C(18,18,28));
    dbdr(x+8,cy,w-16,ca_h,C(0,80,160));
    draw_textbuf(wi,x+8,cy,w-16,ca_h,C_CYAN,C(18,18,28));
    draw_inputbar(wi,x+1,y+h-inph-4,w-2,foc,"$ hexcalc ",C_TPMT);

    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}



static void nano_get_line_info(win_t *wi, int line_num, int *out_start, int *out_len) {
    int cur_line = 0, start = 0, len = 0;
    for(int i = 0; i < wi->blen; i++) {
        char c = wi->buf[(wi->bstart + i) % TBSZ];
        if(c == '\n') {
            if(cur_line == line_num) { *out_start = start; *out_len = len; return; }
            cur_line++; start = i + 1; len = 0;
        } else {
            len++;
        }
    }
    if(cur_line == line_num) { *out_start = start; *out_len = len; }
}


static int nano_count_lines(win_t *wi) {
    int lines = 1;
    for(int i = 0; i < wi->blen; i++) {
        if(wi->buf[(wi->bstart + i) % TBSZ] == '\n') lines++;
    }
    return lines;
}


static void nano_insert_char(win_t *wi, char c) {
    if(wi->blen >= TBSZ - 1) return;

    int line_start = 0, line_len = 0;
    nano_get_line_info(wi, wi->nano_cur_row, &line_start, &line_len);

    int insert_pos = line_start + wi->nano_cur_col;
    if(wi->nano_cur_col > line_len) insert_pos = line_start + line_len;

    if(wi->blen == TBSZ - 1) return;

    for(int i = wi->blen; i > insert_pos; i--) {
        int src_idx = (wi->bstart + i - 1) % TBSZ;
        int dst_idx = (wi->bstart + i) % TBSZ;
        wi->buf[dst_idx] = wi->buf[src_idx];
    }

    wi->buf[(wi->bstart + insert_pos) % TBSZ] = c;
    wi->blen++;
    if(c == '\n') {
        wi->nano_cur_row++;
        wi->nano_cur_col = 0;
    } else {
        wi->nano_cur_col++;
    }
}


static void nano_delete_char(win_t *wi) {
    if(wi->blen == 0) return;

    if(wi->nano_cur_col == 0) {
        if(wi->nano_cur_row == 0) return;
        wi->nano_cur_row--;
        int line_start = 0, line_len = 0;
        nano_get_line_info(wi, wi->nano_cur_row, &line_start, &line_len);
        wi->nano_cur_col = line_len;
    } else {
        wi->nano_cur_col--;
    }

    int line_start = 0, line_len = 0;
    nano_get_line_info(wi, wi->nano_cur_row, &line_start, &line_len);
    int delete_pos = line_start + wi->nano_cur_col;


    for(int i = delete_pos; i < wi->blen - 1; i++) {
        int src_idx = (wi->bstart + i + 1) % TBSZ;
        int dst_idx = (wi->bstart + i) % TBSZ;
        wi->buf[dst_idx] = wi->buf[src_idx];
    }
    wi->blen--;
}


static void drawnano(win_t *wi, int foc) {
    int x=wi->x, y=wi->y, w=wi->w, h=wi->h;
    dr(x+4, y+4, w, h, C_BLACK);
    draw_titlebar(wi, foc, &ICO_NANO, "michealsoft physical studio", 1);


    dr(x, y+TITLE_H, w, 28, C(45,45,48));
    ds(x+10, y+TITLE_H+6, "Physical Studio v2026", C(200,200,200), C(45,45,48));


    int rbx = x+w-80, rby = y+TITLE_H+5, rbw = 70, rbh = 18;
    dr(rbx, rby, rbw, rbh, C(0,122,204));
    ds(rbx+10, rby+2, " START", C_WHITE, C(0,122,204));


    int sby=y+h-22;
    dr(x, sby, w, 22, C(0,122,204));
    ds(x+10, sby+4, "Ready", C_WHITE, C(0,122,204));

    int tay=y+TITLE_H+28, tah=sby-tay;
    dr(x, tay, w, tah, C(30,30,30));

    int lnw=40;
    dr(x, tay, lnw, tah, C(30,30,30));
    dr(x+lnw, tay, 1, tah, C(63,63,70));

    draw_nano_buffer(wi, x, tay+6, w, tah-12, lnw);

    if(foc && (_ticks()/400)&1) {
        int cxp = x+lnw+6+wi->nano_cur_col*FW;
        int ryp = tay+6+wi->nano_cur_row*FH;
        if(cxp < x+w-8 && ryp < sby-8) {
            dr(cxp, ryp, 2, FH, C_WHITE);
        }
    }
    dbdr(x, y, w, h, foc?C(0,122,204):C_WBDR_I);
}



static void drawmorse(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_TOOLS,"Signal Decoder",1);

    int inph=FH+14;
    dr(x,y+TITLE_H,w,h-TITLE_H,C(18,18,28));


    dr(x,y+TITLE_H,w,24,C(28,28,45));
    ds(x+10,y+TITLE_H+4,"CRYSTAL MORSE - [Listening...]", C(160,80,255), C(28,28,45));

    int ca_h=h-TITLE_H-inph-32;
    draw_textbuf(wi,x+8,y+TITLE_H+32,w-16,ca_h,C_CYAN,C(10,10,20));
    dbdr(x+8,y+TITLE_H+32,w-16,ca_h,C_ACCENT);

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_ACCENT);
    ds(x+12,iy+5,"INPUT> ",C_ACCENT,C_TINP);
    dsn(x+12+7*FW,iy+5,wi->inp,wi->inlen,C_WHITE,C_TINP);

    if(foc&&(_ticks()/500)&1) dr(x+12+(7+wi->inlen)*FW,iy+5,2,FH,C_WHITE);

    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawascii(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_TOOLS,"ASCII Table",1);
    dr(x,y+TITLE_H,w,h-TITLE_H,C(18,18,24));
    int hy=y+TITLE_H+4;
    dr(x,hy,w,FH+4,C(28,28,40));
    ds(x+4,hy+2," Dec Hex Chr    Dec Hex Chr    Dec Hex Chr    Dec Hex Chr",C_MGREY,C(28,28,40));
    int row_h=FH+2,col_w=(w-8)/4;
    for(int col=0;col<4;col++){
        for(int row=0;row<32;row++){
            int code=col*32+row+32; if(code>127)break;
            int rx=x+4+col*col_w;
            int ry=hy+FH+6+row*row_h;
            if(ry+row_h>y+h)break;
            uint32_t bg=(row%2==0)?C(22,22,32):C(18,18,28);
            dr(rx,ry,col_w-2,row_h,bg);
            char db[5]; int di=0,dv=code;
            char t3[4]; int ti3=0;
            if(!dv){t3[ti3++]='0';}else{int v3=dv;while(v3){t3[ti3++]='0'+v3%10;v3/=10;}}
            while(ti3>0) { db[di++]=t3[--ti3]; }
            db[di]=0;
            ds(rx+2,ry+1,db,C_LGREY,bg);
            char hb[4]; hb[0]="0123456789ABCDEF"[code>>4]; hb[1]="0123456789ABCDEF"[code&0xF]; hb[2]=0;
            ds(rx+28,ry+1,hb,C(0,180,255),bg);
            char cb2[2]; cb2[0]=(code>=32&&code<127)?(char)code:'.'; cb2[1]=0;
            ds(rx+50,ry+1,cb2,C(0,220,120),bg);
        }
    }
    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawoutput(win_t *wi,int foc,const char *title,const icon_t *ico){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,ico,title,1);
    int inph=FH+8;
    int cah=h-TITLE_H-inph-6;
    dr(x,y+TITLE_H,w,h-TITLE_H,C_WBG);
    draw_textbuf(wi,x+7,y+TITLE_H+6,w-14,cah,C_TGRN,C_WBG);
    draw_inputbar(wi,x+1,y+h-inph,w-2,foc,"$ ",C_TPMT);

    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawgcc(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_GCC,"Compiler Toolchain",0);

    int sby=y+TITLE_H;
    dr(x,sby,w,26,C(14,24,14));
    dbdr(x,sby,w,26,C(0,200,80));
    ds(x+10,sby+5,"GCC EXPERIMENTAL - [i686-TARGET]",C_GREEN,C(14,24,14));

    int inph=FH+12;
    int cah=h-TITLE_H-26-inph-8;
    dr(x,sby+26,w,cah,C(10,12,10));
    draw_textbuf(wi,x+8,sby+30,w-16,cah-8,C_GREEN,C(10,12,10));

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_GREEN);
    ds(x+10,iy+5,"GCC >",C_GREEN,C_TINP);
    dsn(x+10+6*FW,iy+5,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawnetmon(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_NET,"Network HUB",0);


    const char *ntabs[]={"DHCP","Status","Ping","WiFi","DNS","Wget"};
    int taby=y+TITLE_H;
    dr(x,taby,w,26,C(24,18,34));
    int tw=w/6;
    for(int i=0;i<6;i++){
        uint32_t tbg=(i==wi->net_tab)?C_ACCENT:C(32,24,45);
        dr(x+i*tw,taby,tw,26,tbg);
        ds(x+i*tw+8,taby+6,ntabs[i],i==wi->net_tab?C_WHITE:C_MGREY,tbg);
        if(i<5) dr(x+(i+1)*tw-1,taby+4,1,18,C(55,45,75));
    }

    int inph=FH+12, cah=h-TITLE_H-26-inph-6;
    dr(x,taby+26,w,cah,C_NET_BG);
    draw_textbuf(wi,x+8,taby+30,w-16,cah-8,C_CYAN,C_NET_BG);

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_CYAN);
    ds(x+10,iy+6,"NET>",C_CYAN,C_TINP);
    dsn(x+10+5*FW,iy+6,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}

static void drawtaskmon(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_SYSINFO,"Core Monitor",0);

    const char *ttabs[]={"Process","Uptime","Dmesg","ACPI"};
    int taby=y+TITLE_H;
    dr(x,taby,w,26,C(24,24,34));
    int tw=w/4, tsel=wi->net_tab;
    for(int i=0;i<4;i++){
        uint32_t tbg=(i==tsel)?C_ACCENT:C(34,34,44);
        dr(x+i*tw,taby,tw,26,tbg);
        ds(x+i*tw+10,taby+6,ttabs[i],i==tsel?C_WHITE:C_MGREY,tbg);
    }

    int inph=FH+10, cah=h-TITLE_H-26-inph-6;
    dr(x,taby+26,w,cah,C_SYS_BG);
    draw_textbuf(wi,x+8,taby+32,w-16,cah-10,C_WHITE,C_SYS_BG);

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_ACCENT);
    ds(x+10,iy+4,"SYS>",C_ACCENT,C_TINP);
    dsn(x+10+5*FW,iy+4,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawdiskmon(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_SYSINFO,"Disk & Storage",0);

    const char *dtabs[]={"Disk","Memory","CPU","SMP","DF","DU"};
    int taby=y+TITLE_H;
    dr(x,taby,w,26,C(22,22,34));
    int tw=w/6, dsel=wi->net_tab;
    for(int i=0;i<6;i++){
        uint32_t tbg=(i==dsel)?C_ACCENT:C(32,32,45);
        dr(x+i*tw,taby,tw,26,tbg);
        ds(x+i*tw+4,taby+6,dtabs[i],i==dsel?C_WHITE:C_MGREY,tbg);
    }

    int inph=FH+10, cah=h-TITLE_H-26-inph-6;
    dr(x,taby+26,w,cah,C_WBG);
    draw_textbuf(wi,x+8,taby+32,w-16,cah-10,C(120,200,255),C_WBG);

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_ACCENT);
    ds(x+10,iy+4,"DISK>",C_ACCENT,C_TINP);
    dsn(x+10+6*FW,iy+4,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}








static int32_t _fgl_sin(int32_t d){
    static const int16_t T[64]={0,13,25,37,50,62,74,86,98,109,121,132,142,153,163,172,
        181,190,198,205,212,219,225,231,236,240,244,247,250,252,253,254,255,254,253,252,
        250,247,244,240,236,231,225,219,212,205,198,190,181,172,163,153,142,132,121,109,
        98,86,74,62,50,37,25,13};
    d&=0xFF;
    if(d<64)  return T[d];
    if(d<128) return T[127-d];
    if(d<192) return -(int32_t)T[d-128];
    return -(int32_t)T[255-d];
}
static int32_t _fgl_cos(int32_t d){ return _fgl_sin((d+64)&0xFF); }

static void _fgl_line(int x0,int y0,int x1,int y1,uint32_t col){
    int dx=x1-x0; if(dx<0)dx=-dx;
    int dy=y1-y0; if(dy<0)dy=-dy;
    int sx=(x0<x1)?1:-1, sy=(y0<y1)?1:-1, err=dx-dy;
    for(;;){
        vbe_put_pixel(x0,y0,col);
        if(x0==x1&&y0==y1) break;
        int e2=2*err;
        if(e2>-dy){err-=dy;x0+=sx;}
        if(e2< dx){err+=dx;y0+=sy;}
    }
}
static void drawfemboygl_test(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_FEMBOYGL,"FemboyGL Test",0);
    int tby=y+TITLE_H;
    dr(x,tby,w,28,C(30,30,40));
    ds(x+6,tby+6,"femboyGL v1.0 - software rasterizer",C_ACCENT,C(30,30,40));
    int cw2=w-2, ch2=h-TITLE_H-28-2;
    int csx=x+1, csy=tby+28;
    if(cw2<10||ch2<10){dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);return;}

    for(int py=0;py<ch2;py++){
        for(int px=0;px<cw2;px++){
            int r=(px*80)/cw2;
            int g=(py*60)/ch2;
            int b=40+((px+py)*30)/(cw2+ch2);
            if(r>255) r=255;
            if(g>255) g=255;
            if(b>255) b=255;
            vbe_put_pixel(csx+px,csy+py,_GG_COL(r,g,b));
        }
    }

    static int show_stockings = 1;
    static int last_t_key = 0;
    extern char keyboard_peek(void);


    int t_pressed = (foc && keyboard_peek() == 't');
    if (t_pressed && !last_t_key) {
        show_stockings = !show_stockings;
        extern char keyboard_getchar(void); keyboard_getchar();
    }
    last_t_key = t_pressed;

    static float j_y1 = 0, j_v1 = 0;
    static float j_y2 = 0, j_v2 = 0;
    static uint32_t last_t = 0;
    uint32_t now_t = _ticks();
    float dt = (last_t == 0) ? 0.016f : (float)(now_t - last_t) / 1000.0f;
    last_t = now_t;
    if (dt > 0.1f) dt = 0.1f;


    static int last_mx = 0, last_my = 0;
    if (wi->drag && pin(mouse_x, mouse_y, csx, csy, cw2, ch2)) {
        float dx = (float)(mouse_x - last_mx);
        float dy = (float)(mouse_y - last_my);
        j_v1 += dy * 10.0f; j_v2 += dy * 8.0f;
        j_y1 += dx * 0.5f;  j_y2 -= dx * 0.5f;
    }
    last_mx = mouse_x; last_my = mouse_y;

    float k = 180.0f, c = 3.2f, g = 9.8f;
    float wave = _fsin((float)now_t * 0.008f) * 4.0f;

    j_v1 += (-k * j_y1 - c * j_v1 + g * 15.0f + wave) * dt;
    j_y1 += j_v1 * dt;
    j_v2 += (-k * j_y2 - c * j_v2 + g * 15.0f - wave) * dt;
    j_y2 += j_v2 * dt;

    int cx2 = csx + cw2/2, cy2 = csy + ch2/2;
    int base_r = (cw2 < ch2 ? cw2 : ch2) / 5;


    for (int side = -1; side <= 1; side += 2) {
        float jy = (side == -1) ? j_y1 : j_y2;
        int tx = cx2 + side * (base_r - 2);
        int ty = cy2 - 50 + (int)jy;
        int rx = (int)((float)base_r * 1.1f), ry = base_r * 3;

        for (int ey = -ry; ey <= ry; ey++) {
            float y_norm = (float)(ey + ry) / (float)(ry * 2);
            float taper = 1.0f - (y_norm * y_norm * 0.5f);
            if (y_norm < 0.2f) taper *= (0.8f + y_norm * 1.0f);

            int cur_rx = (int)((float)rx * taper);
            for (int ex = -cur_rx; ex <= cur_rx; ex++) {
                int px = tx + ex, py = ty + ey;
                if (px >= csx && px < csx+cw2 && py >= csy && py < csy+ch2) {
                    float depth = (float)(ex*ex) / (float)(cur_rx*cur_rx + 1);
                    int r = 255, g = 215, b = 195;

                    int is_stocking = (show_stockings && ey > -ry/2);
                    int stocking_y = ey - (-ry/2);
                    int is_band = (show_stockings && stocking_y >= 0 && stocking_y < 12);
                    int is_stripe = (show_stockings && is_band && (stocking_y % 4 < 2));

                    if (is_stocking) {
                        r = 25; g = 25; b = 35;
                        if (is_band) {
                            if (is_stripe) { r = 240; g = 240; b = 240; }
                            else { r = 40; g = 10; b = 50; }
                        }
                    }

                    float shade = 1.0f - depth * 0.4f;
                    r = (int)((float)r * shade); g = (int)((float)g * shade); b = (int)((float)b * shade);
                    if (!is_stocking && ey < 0) g += (int)(_fabs(jy)*1.5f);
                    vbe_put_pixel(px, py, _GG_COL(r, g, b));
                }
            }
        }
    }

    ds(csx+4, csy+4, "ThighPhys v1.3 - STRIPED EDITION", C_WHITE, _GG_COL(0,0,0));
    ds(csx+4, csy+16, "Press 'T' to toggle Thigh-Highs", C_LGREY, _GG_COL(0,0,0));
    ds(csx+4, csy+28, "Click and Drag to FLING", C_CYAN, _GG_COL(0,0,0));


    char dbg[64]; _scpy(dbg, "G-Force: ", 10); _itoa(dbg+9, (int)(_fabs(j_v1)));
    ds(csx+4, csy+ch2-14, dbg, C_CYAN, _GG_COL(0,0,0));

    dbdr(csx,csy,cw2,ch2,C_DGREY);
    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}

static void drawdraw(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    draw_state_t *ds2=wi->draw;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_DRAW,"Neo-Paint",0);

    int tby=y+TITLE_H;

    dr(x,tby,w,32,C(28,28,45));
    dr(x,tby+31,w,1,C_ACCENT);


    uint32_t p_bg=(ds2&&ds2->tool==0)?C_ACCENT:C(42,42,60);
    uint32_t e_bg=(ds2&&ds2->tool==1)?C_ACCENT:C(42,42,60);
    dr(x+6,tby+6,56,20,p_bg);
    ds(x+14,tby+8,"PEN",C_WHITE,p_bg);
    dr(x+68,tby+6,56,20,e_bg);
    ds(x+72,tby+8,"ERASE",C_WHITE,e_bg);

    static const uint32_t pal[]={C_WHITE,C_RED,C_GREEN,C_CYAN,C_YELLOW,C_ACCENT,C(255,128,0),C(128,0,255)};
    for(int i=0;i<8;i++){
        dr(x+140+i*24,tby+6,20,20,pal[i]);
        if(ds2 && ds2->fg_color == pal[i]) dbdr(x+140+i*24,tby+6,20,20,C_WHITE);
    }

    int cay=tby+32,caw=w,cah=h-TITLE_H-32;
    if(!ds2){
        dr(x,cay,caw,cah,C(18,12,22));
        ds(x+caw/2-40,cay+cah/2,"[ NO CANVAS ]",C(60,40,80),C(18,12,22));
    } else {
        int pw=caw/DRAW_W,ph=cah/DRAW_H;
        if(pw<1)pw=1;if(ph<1)ph=1;
        dr(x,cay,caw,cah,C(12,12,18));
        for(int r=0;r<DRAW_H;r++){
            for(int c=0;c<DRAW_W;c++){
                if(ds2->pixels[r][c]!=C(30,30,40)){
                    dr(x+c*pw,cay+r*ph,pw,ph,ds2->pixels[r][c]);
                }
            }
        }
    }
    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawirc(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_NET,"CryIRC Client",0);

    int sy=y+TITLE_H;
    dr(x,sy,w,24,C(14,22,44));
    ds(x+8,sy+4,"IRC - irc <server> | /join

    int inph=FH+12, cah=h-TITLE_H-24-inph-6;
    dr(x,sy+24,w,cah,C(8,8,18));
    draw_textbuf(wi,x+8,sy+24,w-16,cah,C_CYAN,C(8,8,18));

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_ACCENT);
    ds(x+10,iy+5,"IRC>",C_ACCENT,C_TINP);
    dsn(x+10+5*FW,iy+5,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawssh(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_NET,"VoidShell SSH",0);

    int sy=y+TITLE_H;
    dr(x,sy,w,24,C(14,32,14));
    ds(x+8,sy+4,"SSH - ssh user@host | ^C Exit",C_GREEN,C(14,32,14));

    int inph=FH+12, cah=h-TITLE_H-24-inph-6;
    dr(x,sy+24,w,cah,C(6,12,6));
    draw_textbuf(wi,x+8,sy+24,w-16,cah,C_GREEN,C(6,12,6));

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_GREEN);
    ds(x+10,iy+5,"SSH>",C_GREEN,C_TINP);
    dsn(x+10+5*FW,iy+5,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawtexttool(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_TOOLS,wi->title,1);
    int inph=FH+8;
    int cah=h-TITLE_H-inph-6;
    dr(x,y+TITLE_H,w,h-TITLE_H,C_WBG);
    draw_textbuf(wi,x+4,y+TITLE_H+4,w-8,cah,C(200,220,255),C_WBG);
    draw_inputbar(wi,x+1,y+h-inph,w-2,foc,"$ ",C_MGREY);

    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}




static void draw_game_chrome(win_t *wi, int foc, const char *title) {
    int x=wi->x, y=wi->y, w=wi->w, h=wi->h;
    dr(x+4, y+4, w, h, C_BLACK);
    draw_titlebar(wi, foc, &ICO_GAME, title, 1);
    int cy=y+TITLE_H;
    dr(x, cy, w, h-TITLE_H, C(12,12,20));

    const char *hint = "Press ANY KEY to launch application...";
    int hlen=0; while(hint[hlen]) hlen++;
    ds(x+(w-hlen*FW)/2, cy+(h-TITLE_H)/2-FH/2, hint, C_LGREY, C(12,12,20));
    dbdr(x, y, w, h, foc ? C_ACCENT : C_WBDR_I);
}

static void drawsnake_win  (win_t *wi, int foc){ draw_game_chrome(wi, foc, "Snake");     }
static void drawtetris_win (win_t *wi, int foc){ draw_game_chrome(wi, foc, "Tetris");    }
static void drawpong_win   (win_t *wi, int foc){ draw_game_chrome(wi, foc, "Pong");      }
static void drawminecraft  (win_t *wi, int foc){ draw_game_chrome(wi, foc, "Minecraft"); }





static void drawtermthemed(win_t *wi, int foc,
                            const icon_t *ico,
                            const char *subtitle,
                            uint32_t hdr_bg,
                            uint32_t hdr_fg,
                            uint32_t content_bg,
                            uint32_t text_col,
                            uint32_t border_accent){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,ico,wi->title,0);


    int sby=y+TITLE_H;
    dr(x,sby,w,22,hdr_bg);
    dr(x,sby,3,22,border_accent);
    ds(x+10,sby+4,subtitle,hdr_fg,hdr_bg);

    int inph=FH+10, cah=h-TITLE_H-22-inph-4;
    dr(x,sby+22,w,cah<0?0:cah,content_bg);
    draw_textbuf(wi,x+8,sby+22,w-16,cah,text_col,content_bg);

    int iy=y+h-inph-2;
    dr(x+2,iy,w-4,inph,C_TINP);
    dbdr(x+2,iy,w-4,inph,border_accent);
    ds(x+10,iy+3,"> ",border_accent,C_TINP);
    dsn(x+10+2*FW,iy+3,wi->inp,wi->inlen,C_WHITE,C_TINP);


    dbdr(x,y,w,h,foc?border_accent:C_WBDR_I);
}


static void drawmatrix(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_GAME,"Digital Rain — type 'matrix' to run",
        C(0,18,0),C(0,180,60),C(2,10,2),C(0,220,65),C(0,200,60));
}
static void drawnyan(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_GAME,"Nyan Cat Animation",
        C(30,10,40),C(255,120,220),C(20,8,28),C(255,160,240),C(200,80,255));
}
static void drawpipes(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_GAME,"Pipes Screensaver",
        C(10,18,30),C(60,180,255),C(8,12,22),C(80,200,255),C(0,120,230));
}
static void drawdoomscroll(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_GAME,"Doomscroll",
        C(28,8,8),C(220,100,60),C(18,5,5),C(200,80,40),C(200,60,0));
}
static void drawspinning(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_FEMBOYGL,"Spinning Cube  (3D wireframe)",
        C(20,10,36),C(160,80,255),C(12,6,24),C(180,100,255),C(120,60,220));
}
static void drawsl(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_GAME,"SL — Steam Locomotive",
        C(28,16,8),C(200,120,60),C(18,10,4),C(200,140,80),C(180,80,0));
}
static void drawrickroll(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_GAME,"Never Gonna Give You Up",
        C(10,10,28),C(100,160,255),C(6,6,20),C(120,180,255),C(60,120,255));
}
static void drawwget(win_t *wi,int foc){
    drawtermthemed(wi,foc,&ICO_NET,"Wget — HTTP downloader  (wget <url>)",
        C(8,26,28),C(0,210,220),C(5,16,18),C(0,220,230),C(0,180,200));
}
static void load_image(win_t *wi) {
    sfs_node_t *n = sfs_resolve(sfs_root(), wi->loaded_file);
    if (!n || n->type != SFS_FILE) return;

    const char *data = (const char*)n->data;
    if (data[0] != 'P' || data[1] != '6') return;

    size_t pos = 3;
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
    int w = 0; while(pos < n->size && data[pos] >= '0' && data[pos] <= '9') { w = w*10 + (data[pos]-'0'); pos++; }
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
    int h = 0; while(pos < n->size && data[pos] >= '0' && data[pos] <= '9') { h = h*10 + (data[pos]-'0'); pos++; }
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
    while(pos < n->size && data[pos] >= '0' && data[pos] <= '9') pos++;
    while(pos < n->size && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;

    if (w <= 0 || h <= 0 || pos >= n->size) return;

    wi->image_data = (uint32_t*)kmalloc(w * h * 4);
    if (!wi->image_data) return;
    wi->image_w = w; wi->image_h = h;

    for (int i = 0; i < w * h; i++) {
        if (pos + 3 > n->size) break;
        uint8_t r = data[pos++];
        uint8_t g = data[pos++];
        uint8_t b = data[pos++];
        wi->image_data[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}

static void drawimageview(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,&ICO_DRAW,"Crystal Viewer",1);

    int chy=y+TITLE_H;
    dr(x,chy,w,26,C(32,24,42));
    ds(x+10,chy+6,"IMAGE VIEWER - ^U Gallery  |  PPM / P6",C(180,140,220),C(32,24,42));

    int cv_y=chy+26, cv_h=h-TITLE_H-26;
    dr(x,cv_y,w,cv_h<0?0:cv_h,C(18,12,22));

    if (wi->loaded_file[0] && !wi->image_data) {
        load_image(wi);
    }

    if (wi->image_data) {
        int iw = wi->image_w, ih = wi->image_h;
        int scale_w = (iw > w) ? w : iw;
        int scale_h = (ih > cv_h) ? cv_h : ih;
        vbe_bb_blit_rect(x + (w - scale_w)/2, cv_y + (cv_h - scale_h)/2, scale_w, scale_h, wi->image_data);
    } else {
        int fx=x+w/2-80, fy=cv_y+cv_h/2-60;
        dr(fx,fy,160,120,C(24,18,30));
        dbdr(fx,fy,160,120,C_ACCENT);
        ds(fx+24,fy+52,wi->loaded_file[0] ? "[ LOAD FAIL ]" : "[ EMPTY VAULT ]",C(60,40,80),C(24,18,30));
    }

    if(wi->loaded_file[0]) ds(x+10,cv_y+10,wi->loaded_file,C_CYAN,C(18,12,22));
    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}

static void drawmediaplayer(win_t *wi, int foc) {
    int x=wi->x, y=wi->y, w=wi->w, h=wi->h;
    dr(x+4, y+4, w, h, C_BLACK);
    draw_titlebar(wi, foc, &ICO_GAME, "Gravity Media", 1);


    dr(x,y+TITLE_H,w,24,C(42,12,64));

    const char *status = wi->loaded_file[0] ? "PLAYING" : "STANDBY";
    char hdr[128];
    _scpy(hdr, "MEDIA PLAYER - [", 128);
    _scat(hdr, status, 128);
    _scat(hdr, "]", 128);
    ds(x+10,y+TITLE_H+4,hdr,C(255,100,200),C(42,12,64));

    int cy=y+TITLE_H+24,ch=h-TITLE_H-24-44;
    dr(x,cy,w,ch<0?0:ch,C(22,8,30));


    int dx=x+w/2,dy=cy+ch/2;
    uint32_t ring=((_ticks()/400)&1)?C_ACCENT:C(100,20,150);
    dr(dx-50,dy-50,100,100,C(12,6,18));
    dbdr(dx-50,dy-50,100,100,ring);
    ds(dx-45,dy-4,"MEDIA PLAYER",ring,C(12,6,18));

    if(wi->loaded_file[0]) {
        int flen = _slen(wi->loaded_file);
        ds(x+(w-flen*FW)/2, cy+10, wi->loaded_file, C_WHITE, C(22,8,30));
    }


    int fy=y+h-44;
    dr(x, fy, w, 44, C(28,28,45));
    dr(x, fy, w, 2, C_ACCENT);
    ds(x+w/2-36, fy+14, "[<<]  PAUSE  [>>]", C(250,250,255), C(28,28,45));

    dbdr(x, y, w, h, foc ? C_ACCENT : C_WBDR_I);
}



static void drawterm(win_t *wi,int foc){
    int x=wi->x,y=wi->y,w=wi->w,h=wi->h;
    dr(x+4,y+4,w,h,C_BLACK);
    draw_titlebar(wi,foc,(wi->app_type==WAPP_TASKMON)?&ICO_SYSINFO:&ICO_TERM,"Root Console",0);

    dr(x,y+TITLE_H,w,h-TITLE_H,C(8,8,12));

    int inph=FH+12,cah=h-TITLE_H-inph-8;
    draw_textbuf(wi,x+8,y+TITLE_H+8,w-16,cah,C_ACCENT,C(12,8,16));

    int iy=y+h-inph-4;
    dr(x+4,iy,w-8,inph,C_TINP);
    dbdr(x+4,iy,w-8,inph,C_ACCENT);
    ds(x+11,iy+6,"stinger>",C_ACCENT,C_TINP);
    dsn(x+11+9*FW,iy+6,wi->inp,wi->inlen,C_WHITE,C_TINP);

    if(foc&&(_ticks()/400)&1) dr(x+11+(9+wi->inlen)*FW,iy+6,2,FH,C_WHITE);

    dbdr(x,y,w,h,foc?C_WBDR_A:C_WBDR_I);
}


static void drawwin(win_t *wi,int foc){
    if(!wi->alive)return;


    if(wi->app_type == WAPP_MODULAR || wi->app_iface){
        AppInterface *ai = (AppInterface*)wi->app_iface;
        if(ai && ai->on_draw) {
            ai->on_draw((struct Window*)wi, ai->app_data);
            return;
        }
    }

    if(wi->app_iface){
        AppInterface *ai = (AppInterface*)wi->app_iface;
        if(ai->on_draw) ai->on_draw((struct Window*)wi, ai->app_data);
        return;
    }

    switch(wi->app_type){
    case WAPP_FILEMANAGER: drawfm(wi,foc);       return;
    case WAPP_BROWSER:     drawbr(wi,foc);        return;
    case WAPP_SETTINGS:    drawsettings(wi,foc);  return;
    case WAPP_CALC:        drawcalc(wi,foc);      return;
    case WAPP_HEXCALC:     drawhexcalc(wi,foc);   return;
    case WAPP_NANO:        drawnano(wi,foc);       return;
    case WAPP_MORSE:       drawmorse(wi,foc);      return;
    case WAPP_ASCII:       drawascii(wi,foc);      return;
    case WAPP_DRAW:        drawdraw(wi,foc);       return;
    case WAPP_GCC:         drawgcc(wi,foc);        return;
    case WAPP_NETWORK:     drawnetmon(wi,foc);     return;
    case WAPP_TASKMON:     drawtaskmon(wi,foc);    return;
    case WAPP_DISKMON:     drawdiskmon(wi,foc);    return;
    case WAPP_TEXT:        drawtexttool(wi,foc);   return;
    case WAPP_IMAGEVIEW:   drawimageview(wi,foc);  return;
    case WAPP_MEDIAPLAYER: drawmediaplayer(wi,foc); return;
    case WAPP_FEMBOYGL_TEST: drawfemboygl_test(wi,foc); return;
    case WAPP_SNAKE:       drawsnake_win(wi,foc);  return;
    case WAPP_TETRIS:      drawtetris_win(wi,foc); return;
    case WAPP_PONG:        drawpong_win(wi,foc);   return;
    case WAPP_MINECRAFT:   drawminecraft(wi,foc);  return;
    case WAPP_MATRIX:      drawmatrix(wi,foc);      return;
    case WAPP_NYAN:        drawnyan(wi,foc);        return;
    case WAPP_PIPES:       drawpipes(wi,foc);       return;
    default:               drawterm(wi,foc);       return;
    }
}


static void dokey(char k){
    if(g_focus<0||g_focus>=g_nwins||!g_wins[g_focus].alive)return;
    win_t *wi=&g_wins[g_focus];


    if(wi->file_picker && wi->file_picker->active){
        if(k=='\x1b'){wi->esclen=1;wi->esc[0]=k;return;}
        if(wi->esclen>0){
            wi->esc[wi->esclen++]=k;
            if(wi->esclen==3){
                if(wi->esc[1]=='['){
                    char dir=wi->esc[2];
                    if(dir=='A'&&wi->file_picker->sel>0){wi->file_picker->sel--;if(wi->file_picker->sel<wi->file_picker->scroll)wi->file_picker->scroll=wi->file_picker->sel;}
                    if(dir=='B'&&wi->file_picker->sel<wi->file_picker->entry_count-1){wi->file_picker->sel++;if(wi->file_picker->sel>=wi->file_picker->scroll+10)wi->file_picker->scroll=wi->file_picker->sel-9;}
                }
                wi->esclen=0;
            }
            return;
        }
        if(k=='\n'||k=='\r'){

            if(wi->file_picker->input_len>0){
                wi->file_picker->input[wi->file_picker->input_len]=0;
                _scpy(wi->loaded_file,wi->file_picker->input,256);
                wi->file_picker->result_confirmed=(wi->file_picker->mode==FP_MODE_SAVE)?2:1;
                wi->file_picker->active=0;
                return;
            }

            int sel=wi->file_picker->sel;
            if(sel>=0&&sel<wi->file_picker->entry_count){
                if(wi->file_picker->is_dir[sel]){
                    fp_navigate(wi->file_picker,sel,wi->app_type);
                } else {
                    _scpy(wi->loaded_file,wi->file_picker->entries[sel],256);
                    wi->file_picker->result_confirmed=1;
                    wi->file_picker->active=0;
                }
            }
            return;
        }
        if(k=='\b'){if(wi->file_picker->input_len>0)wi->file_picker->input_len--; return;}
        if(k==27){wi->file_picker->active=0;return;}
        if(k>=32&&k<127){if(wi->file_picker->input_len<127)wi->file_picker->input[wi->file_picker->input_len++]=k;}
        return;
    }





    if(k=='\x1b'){wi->esclen=1;wi->esc[0]=k;return;}
    if(wi->esclen>0){
        wi->esc[wi->esclen++]=k;
        if(wi->esclen==3){
            if(wi->esc[1]=='['){
                char dir=wi->esc[2];
                if(wi->app_type==WAPP_FILEMANAGER){
                    if(dir=='A'&&wi->fm_sel>0){wi->fm_sel--;if(wi->fm_sel<wi->fm_scroll)wi->fm_scroll=wi->fm_sel;}
                    if(dir=='B'&&wi->fm_sel<wi->fm_count-1){wi->fm_sel++;if(wi->fm_sel>=wi->fm_scroll+10)wi->fm_scroll=wi->fm_sel-9;}
                }
            }
            wi->esclen=0;
        }
        return;
    }

    if(wi->app_type==WAPP_MINECRAFT){
        if(k=='e'||k=='E'){
            wi->settings_tab = !wi->settings_tab;
        }
        return;
    }

    if(wi->app_type==WAPP_BROWSER){
        if(k=='\n'||k=='\r'){br_fetch(wi);return;}
        if(k=='\b'){int l=0;while(wi->br_url[l])l++;if(l>0)wi->br_url[l-1]=0;return;}
        if(k>=32&&k<127){int l=0;while(wi->br_url[l])l++;if(l<253){wi->br_url[l]=k;wi->br_url[l+1]=0;}return;}
        return;
    }
    if(wi->app_type==WAPP_FILEMANAGER){
        if(k=='\n'||k=='\r'){fm_navigate(wi,wi->fm_sel);return;}
        if(k=='\b'){fm_navigate(wi,0);return;}
        return;
    }
    if(wi->app_type==WAPP_SETTINGS||wi->app_type==WAPP_SYSINFO||wi->app_type==WAPP_ASCII)return;

    if(wi->app_type==WAPP_CALC){
        if((k>='0'&&k<='9')||k=='+'||k=='-'||k=='*'||k=='/'||k=='('||k==')'||k=='.'||k=='^'||k=='%'){
            if(wi->inlen<INPUT_MAX){wi->inp[wi->inlen++]=k;}
        } else if(k=='c'||k=='C'){wi->inlen=0;
        } else if(k=='\b'&&wi->inlen>0){wi->inlen--;
        } else if(k=='='||k=='\n'||k=='\r'){
            wi->inp[wi->inlen]=0;
            char cmd[64]="calc "; int ci2=5;
            for(int i=0;i<wi->inlen&&ci2<62;i++) cmd[ci2++]=wi->inp[i];
            cmd[ci2]=0;
            wwrite(wi,"\n");
            run_cmd(wi,cmd);
            wi->inlen=0;
        }
        return;
    }


    if(wi->app_type==WAPP_MORSE||wi->app_type==WAPP_FIGLET||wi->app_type==WAPP_COWSAY||
       wi->app_type==WAPP_HEXCALC||wi->app_type==WAPP_TEXT){
        if(k=='\n'||k=='\r'){
            wi->inp[wi->inlen]=0;
            const char *prefix=
                wi->app_type==WAPP_MORSE   ?"morse ":
                wi->app_type==WAPP_FIGLET  ?"figlet ":
                wi->app_type==WAPP_COWSAY  ?"cowsay ":
                wi->app_type==WAPP_HEXCALC ?"hexcalc ":
                wi->app_type==WAPP_TEXT    ?wi->queued_cmd:
                "";
            char cmd[280]; int ci2=0;
            for(int i=0;prefix[i]&&ci2<270;i++) cmd[ci2++]=prefix[i];
            for(int i=0;i<wi->inlen&&ci2<278;i++) cmd[ci2++]=wi->inp[i];
            cmd[ci2]=0;
            wwrite(wi,"\n");
            run_cmd(wi,cmd);
            wi->inlen=0;
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(k==21){
        if(wi->file_picker){wi->file_picker->active=1; fp_populate(wi->file_picker, wi->app_type);}
        else {
            wi->file_picker=fp_create(FP_MODE_OPEN,"Open File");
            if(wi->file_picker){fp_populate(wi->file_picker, wi->app_type);wi->file_picker->active=1;}
        }
        return;
    }

    if(k==15){
        if(wi->file_picker){wi->file_picker->mode=FP_MODE_SAVE;wi->file_picker->active=1; fp_populate(wi->file_picker, wi->app_type);}
        else {
            wi->file_picker=fp_create(FP_MODE_SAVE,"Save File");
            if(wi->file_picker){fp_populate(wi->file_picker, wi->app_type);wi->file_picker->active=1;}
        }
        return;
    }

    if(wi->app_type==WAPP_NANO){
        if(k==24){wi->alive=0;g_focus=(g_nwins>=2)?g_nwins-2:-1;return;}
        if(k==18){
            smact_run(wi, "save tmp.py");
            run_cmd(wi, "python tmp.py");
            return;
        }
        if(k=='\n'||k=='\r'){nano_insert_char(wi,'\n');return;}
        if(k=='\b'){nano_delete_char(wi);return;}
        if(k>=32&&k<127){nano_insert_char(wi,k);return;}

        if(k=='\x1b'){wi->esclen=1;wi->esc[0]=k;return;}
        if(wi->esclen>0){
            wi->esc[wi->esclen++]=k;
            if(wi->esclen==3){
                if(wi->esc[1]=='['){
                    char dir=wi->esc[2];
                    int nlines=nano_count_lines(wi);
                    if(dir=='A'){
                        if(wi->nano_cur_row>0){
                            wi->nano_cur_row--;
                            int ls,ll;
                            nano_get_line_info(wi,wi->nano_cur_row,&ls,&ll);
                            if(wi->nano_cur_col>ll)wi->nano_cur_col=ll;
                        }
                    }
                    if(dir=='B'){
                        if(wi->nano_cur_row<nlines-1){
                            wi->nano_cur_row++;
                            int ls,ll;
                            nano_get_line_info(wi,wi->nano_cur_row,&ls,&ll);
                            if(wi->nano_cur_col>ll)wi->nano_cur_col=ll;
                        }
                    }
                    if(dir=='C'){
                        int ls,ll;
                        nano_get_line_info(wi,wi->nano_cur_row,&ls,&ll);
                        if(wi->nano_cur_col<ll)wi->nano_cur_col++;
                    }
                    if(dir=='D'){
                        if(wi->nano_cur_col>0)wi->nano_cur_col--;
                    }
                }
                wi->esclen=0;
            }
            return;
        }
        return;
    }


    if(wi->app_type==WAPP_GCC){
        if(k=='\n'||k=='\r'){
            wi->inp[wi->inlen]=0;
            char cmd[280]="gcc "; int ci2=4;
            for(int i=0;i<wi->inlen&&ci2<278;i++)cmd[ci2++]=wi->inp[i]; cmd[ci2]=0;
            wwrite(wi,"$ gcc ");wwrite(wi,wi->inp);wwrite(wi,"\n");
            run_cmd(wi,cmd);
            wi->inlen=0;
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(wi->app_type==WAPP_NETWORK){
        if(k=='\n'||k=='\r'){
            const char *cmds[]={"dhcp","ifconfig","ping 8.8.8.8","wifi status","nslookup google.com","wget"};
            if(wi->inlen>0){
                wi->inp[wi->inlen]=0;
                wwrite(wi,"$ ");wwrite(wi,wi->inp);wwrite(wi,"\n");
                run_cmd(wi,wi->inp);
                wi->inlen=0;
            } else {
                int t=wi->net_tab; if(t<0)t=0; if(t>5)t=5;
                wwrite(wi,"$ ");wwrite(wi,cmds[t]);wwrite(wi,"\n");
                run_cmd(wi,cmds[t]);
            }
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(wi->app_type==WAPP_TASKMON){
        if(k=='\n'||k=='\r'){
            const char *cmds[]={"ps","uptime","dmesg","acpi"};
            if(wi->inlen>0){
                wi->inp[wi->inlen]=0;
                wwrite(wi,"$ ");wwrite(wi,wi->inp);wwrite(wi,"\n");
                run_cmd(wi,wi->inp);
                wi->inlen=0;
            } else {
                int t=wi->net_tab; if(t<0)t=0; if(t>3)t=3;
                wwrite(wi,"$ ");wwrite(wi,cmds[t]);wwrite(wi,"\n");
                run_cmd(wi,cmds[t]);
            }
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(wi->app_type==WAPP_DISKMON){
        if(k=='\n'||k=='\r'){
            const char *cmds[]={"diskinfo","meminfo","lscpu","smpinfo","df","du"};
            if(wi->inlen>0){
                wi->inp[wi->inlen]=0;
                wwrite(wi,"$ ");wwrite(wi,wi->inp);wwrite(wi,"\n");
                run_cmd(wi,wi->inp);
                wi->inlen=0;
            } else {
                int t=wi->net_tab; if(t<0)t=0; if(t>5)t=5;
                wwrite(wi,"$ ");wwrite(wi,cmds[t]);wwrite(wi,"\n");
                run_cmd(wi,cmds[t]);
            }
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(wi->app_type==WAPP_IRC){
        if(k=='\n'||k=='\r'){
            wi->inp[wi->inlen]=0;
            if(wi->inlen>0){
                wwrite(wi,"IRC> ");wwrite(wi,wi->inp);wwrite(wi,"\n");
                char cmd[280]="irc "; int ci2=4;
                for(int i=0;i<wi->inlen&&ci2<278;i++)cmd[ci2++]=wi->inp[i]; cmd[ci2]=0;
                run_cmd(wi,cmd);
                wi->inlen=0;
            }
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(wi->app_type==WAPP_SSH){
        if(k=='\n'||k=='\r'){
            wi->inp[wi->inlen]=0;
            if(wi->inlen>0){
                wwrite(wi,"SSH> ");wwrite(wi,wi->inp);wwrite(wi,"\n");
                char cmd[280]="ssh "; int ci2=4;
                for(int i=0;i<wi->inlen&&ci2<278;i++)cmd[ci2++]=wi->inp[i]; cmd[ci2]=0;
                run_cmd(wi,cmd);
                wi->inlen=0;
            }
        } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
        } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
        return;
    }


    if(k==3){
        extern volatile int g_net_cancel;
        g_net_cancel=1;
        wwrite(wi,"^C\n$ ");wi->inlen=0;return;
    }
    if(k=='\n'||k=='\r'){
        wwrite(wi,"\n");
        if(wi->inlen>0){
            wi->inp[wi->inlen]=0;
            int is_clear=0;
            if((wi->inp[0]=='c'&&wi->inp[1]=='l')&&
               ((wi->inp[2]=='e'&&wi->inp[3]=='a'&&wi->inp[4]=='r'&&wi->inp[5]==0)||
                (wi->inp[2]=='s'&&wi->inp[3]==0)))is_clear=1;
            if(is_clear){twin_clear(wi);}
            else{ run_cmd(wi,wi->inp); }
            wi->inlen=0;
        }
        wwrite(wi,"$ ");
    } else if(k=='\b'){if(wi->inlen>0)wi->inlen--;
    } else if(k>=32&&k<127){if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=k;}
}


static void doscroll(int d){
    if(g_focus<0||g_focus>=g_nwins||!g_wins[g_focus].alive)return;
    win_t *wi=&g_wins[g_focus];
    if(wi->app_type==WAPP_FILEMANAGER){wi->fm_scroll+=d;if(wi->fm_scroll<0)wi->fm_scroll=0;if(wi->fm_scroll>=wi->fm_count)wi->fm_scroll=wi->fm_count-1;return;}
    if(wi->app_type==WAPP_BROWSER){wi->br_scroll+=d;if(wi->br_scroll<0)wi->br_scroll=0;return;}
    wi->scroll+=d;if(wi->scroll<0)wi->scroll=0;
}

static void totop(int idx){
    if(idx<0||idx>=g_nwins)return;
    win_t tmp=g_wins[idx];
    for(int i=idx;i<g_nwins-1;i++)g_wins[i]=g_wins[i+1];
    g_wins[g_nwins-1]=tmp;g_focus=g_nwins-1;
}











static const char *sm_cats[SMCAT_COUNT]={
    "  Apps","  Games","  Tools","  Network","  System","  Power"
};
static const uint32_t sm_cat_accent[SMCAT_COUNT]={
    0x0078D7,0x107C10,0xCA5010,0x00AABB,0x767676,0x8B0000
};
static const icon_t *sm_cat_icons[SMCAT_COUNT]={
    &ICO_TERM,&ICO_GAME,&ICO_TOOLS,&ICO_NET,&ICO_SYSINFO,&ICO_POWER
};
typedef struct{const char *name;int id;}sm_app_t;
static const sm_app_t sm_apps[SMCAT_COUNT][16]={

    {{"Terminal",0},{"File Manager",1},{"Browser",2},{"System Info",3},{"Settings",4},
     {"michealsoft physical studio",10},{"Calculator",11},{"Draw",13},{"GCC Compiler",45},
     {"Image Viewer",33},{"Media Player",34},{"FemboyGL Test",38},{0}},

    {{"Snake",71},{"Tetris",72},{"Pong",73},{"Minecraft",74},{"Matrix",63},{"Nyan Cat",64},
     {"Pipes",65},{"Doomscroll",66},{"Spinning Cube",67},{"SL Train",68},{"Rickroll",69},{0}},

    {{"Calculator",11},{"Hex Calc",30},{"Base64",31},{"Morse Code",32},
     {"ASCII Table",33},{"Figlet Banner",55},{"Cowsay",36},{"Rot13",37},
     {"Wget",70},{"michealsoft physical studio",10},{"GCC Compiler",45},{0}},
    {{"Network Monitor",40},{"IRC Client",44},{"SSH Client",48},{"Wget",70},
     {"WiFi",47},{0}},

    {{"Settings",4},{"System Info",3},{"Task Monitor",50},{"Disk/Sys Info",51},
     {"Uptime",56},{"Dmesg",57},{0}},

    {{"Exit to CLI",62},{"Shut Down",60},{"Restart",61},{0}}
};
static const int sm_app_counts[SMCAT_COUNT]={12,11,11,5,6,3};

static int g_smenu=0;
static int g_sm_cat=0;
static int g_sm_hov_cat=-1;
static int g_sm_hov_app=-1;
static int g_stho=0;

static void smenu_hittest(int mx,int my){
    if(!g_smenu){g_sm_hov_cat=-1;g_sm_hov_app=-1;return;}
    int sh=vbe_screen_height;
    int smy=sh-TBAR_H-SM_TOTAL_H;
    int cat_x=SM_ICON_W,app_x=SM_ICON_W+SM_CAT_W;
    g_sm_hov_cat=-1;g_sm_hov_app=-1;
    if(mx>=cat_x&&mx<app_x&&my>=smy+SM_HDR_H){
        int idx=(my-(smy+SM_HDR_H))/SM_ITEMH;
        if(idx>=0&&idx<SMCAT_COUNT)g_sm_hov_cat=idx;
    }
    if(mx>=app_x&&mx<app_x+SM_APP_W&&my>=smy+SM_HDR_H){
        int idx=(my-(smy+SM_HDR_H))/SM_ITEMH;
        if(idx>=0&&idx<sm_app_counts[g_sm_cat])g_sm_hov_app=idx;
    }
}

static void drawsmenu(void){
    if(!g_smenu)return;
    int sh=vbe_screen_height;
    int smy=sh-TBAR_H-SM_TOTAL_H;
    dr(4,smy+4,SMWIDTH,SM_TOTAL_H,C(0,0,0));

    dr(0,smy,SM_ICON_W,SM_TOTAL_H,C(14,14,18));
    dr(SM_ICON_W-1,smy,1,SM_TOTAL_H,C(32,32,40));


    dr(10,smy+12,32,32,C_SM_SEL);
    dbdr(10,smy+12,32,32,C_WHITE);
    ds(17,smy+21,"YK",C_WHITE,C_SM_SEL);


    dricon((SM_ICON_W-16)/2,smy+SM_TOTAL_H-80, &ICO_SETTINGS);
    dricon((SM_ICON_W-16)/2,smy+SM_TOTAL_H-44, &ICO_POWER);


    int cx=SM_ICON_W;
    dr(cx,smy,SM_CAT_W,SM_TOTAL_H,C_SM_M);
    dr(cx,smy,SM_CAT_W,SM_HDR_H,C_SM_HDR);
    ds(cx+12,smy+18,"Navigation",C(110,110,140),C_SM_HDR);
    dr(cx,smy+SM_HDR_H-1,SM_CAT_W,1,C(42,42,52));

    for(int i=0;i<SMCAT_COUNT;i++){
        int iy=smy+SM_HDR_H+i*SM_ITEMH;
        int sel=(i==g_sm_cat),hov=(i==g_sm_hov_cat);
        uint32_t bg=sel?C_SM_SEL:hov?C_SM_HOV:C_SM_M;
        dr(cx,iy,SM_CAT_W,SM_ITEMH,bg);
        if(sel)dr(cx,iy,4,SM_ITEMH,C_WHITE);
        dricon(cx+10,iy+(SM_ITEMH-16)/2,sm_cat_icons[i]);
        ds(cx+32,iy+(SM_ITEMH-FH)/2,sm_cats[i],sel||hov?C_WHITE:C_SM_TXT,bg);
    }
    ds(cx+6,smy+SM_TOTAL_H-16,"Win: toggle menu",C(70,70,70),C_SM_M);
    int ax=SM_ICON_W+SM_CAT_W;
    dr(ax,smy,SM_APP_W,SM_TOTAL_H,C_SM_R);
    uint32_t hcc=C((sm_cat_accent[g_sm_cat]>>16)&0xFF,(sm_cat_accent[g_sm_cat]>>8)&0xFF,sm_cat_accent[g_sm_cat]&0xFF);
    dr(ax,smy,SM_APP_W,SM_HDR_H,hcc);
    dricon(ax+8,smy+8,sm_cat_icons[g_sm_cat]);
    ds(ax+30,smy+18,sm_cats[g_sm_cat],C_WHITE,hcc);
    dr(ax,smy+SM_HDR_H-1,SM_APP_W,1,C(55,55,55));
    int n=sm_app_counts[g_sm_cat];
    for(int i=0;i<n;i++){
        const sm_app_t *e=&sm_apps[g_sm_cat][i];
        if(!e->name)break;
        int iy=smy+SM_HDR_H+i*SM_ITEMH;
        int hov=(i==g_sm_hov_app);
        uint32_t bg=hov?C_SM_HOV:C_SM_R;
        dr(ax,iy,SM_APP_W,SM_ITEMH,bg);
        dricon(ax+8,iy+(SM_ITEMH-16)/2,app_icon(e->id));
        ds(ax+28,iy+(SM_ITEMH-FH)/2,e->name,hov?C_WHITE:C_LGREY,bg);
    }
    dbdr(0,smy,SMWIDTH,SM_TOTAL_H,C(60,60,60));
}


static void smact(int item){
    g_smenu=0;
    switch(item){
    case 0:{win_t *w=walloc("Terminal",70,60,760,500);
        if(w){w->app_type=WAPP_TERMINAL;wwrite(w,"stinger v4.0 — YaoiGUI\n$ ");}break;}
    case 1:{win_t *w=walloc("File Manager",100,60,600,440);
        if(w){w->app_type=WAPP_FILEMANAGER;w->fm_path[0]='/';w->fm_path[1]=0;fm_populate(w);}break;}
    case 2:{win_t *w=walloc("Browser",80,50,720,520);
        if(w){w->app_type=WAPP_BROWSER;const char*u="http://example.com";int i=0;
        while(u[i]&&i<254){w->br_url[i]=u[i];i++;}w->br_url[i]=0;}break;}
    case 3:{win_t *w=walloc("System Info",150,100,560,380);
        if(w){w->app_type=WAPP_SYSINFO;
        smact_run(w,"uname");smact_run(w,"uptime");smact_run(w,"free");smact_run(w,"lscpu");
        wwrite(w,"$ ");}break;}
    case 4:{win_t *w=walloc("Settings",100,60,640,480);
        if(w){w->app_type=WAPP_SETTINGS;w->settings_tab=0;}break;}
    case 10:{win_t *w=walloc("michealsoft physical studio",80,60,780,520);
        if(w){w->app_type=WAPP_NANO;w->nano_cur_row=0;w->nano_cur_col=0;}}break;
    case 11:{win_t *w=walloc("Calculator",200,100,320,360);
        if(w){w->app_type=WAPP_CALC;wwrite(w,"0");}break;}
    case 13:{win_t *w=walloc("Draw",80,60,760,520);
        if(w){w->app_type=WAPP_DRAW;
        w->draw=(draw_state_t*)kmalloc(sizeof(draw_state_t));
        if(w->draw){
            memset(w->draw,0,sizeof(draw_state_t));
            for(int py=0;py<DRAW_H;py++)
                for(int px2=0;px2<DRAW_W;px2++)
                    w->draw->pixels[py][px2]=C(30,30,40);
            w->draw->fg_color=C_WHITE;
            w->draw->tool=0;
        }}}break;
    case 63:{win_t *w=walloc("Matrix",80,60,760,500);
        if(w){w->app_type=WAPP_MATRIX;smact_run(w,"matrix");}break;}
    case 64:{win_t *w=walloc("Nyan Cat",80,60,760,500);
        if(w){w->app_type=WAPP_NYAN;smact_run(w,"nyan");}break;}
    case 65:{win_t *w=walloc("Pipes",80,60,760,500);
        if(w){w->app_type=WAPP_PIPES;smact_run(w,"pipes");}break;}
    case 66:{win_t *w=walloc("Doomscroll",80,60,760,500);
        if(w){w->app_type=WAPP_DOOMSCROLL;smact_run(w,"doomscroll");}break;}
    case 67:{win_t *w=walloc("Spinning Cube",80,60,500,500);
        if(w){w->app_type=WAPP_SPINNING;smact_run(w,"spinning");}break;}
    case 68:{win_t *w=walloc("SL Train",80,60,760,300);
        if(w){w->app_type=WAPP_SL;smact_run(w,"sl");}break;}
    case 69:{win_t *w=walloc("Rickroll",80,60,760,500);
        if(w){w->app_type=WAPP_RICKROLL;smact_run(w,"rickroll");}break;}
    case 70:{win_t *w=walloc("Wget",120,80,600,340);
        if(w){w->app_type=WAPP_WGET;wwrite(w,"Wget — use: wget <url>\n$ ");}break;}

    case 71:{

        win_t *w=walloc("Snake",80,60,640,480);
        if(w){
            w->app_type=WAPP_SNAKE;

            drawsnake_win(w,1);
            vbe_flip_rect(w->x,w->y,w->w,w->h);

            cmd_snake_gui(w->x,w->y,w->w,w->h);

            w->alive=0;
            g_focus=(g_nwins>=2)?g_nwins-2:-1;
        }
        break;}
    case 72:{

        win_t *w=walloc("Tetris",80,60,640,480);
        if(w){
            w->app_type=WAPP_TETRIS;
            drawtetris_win(w,1);
            vbe_flip_rect(w->x,w->y,w->w,w->h);
            cmd_tetris_gui(w->x,w->y,w->w,w->h);
            w->alive=0;
            g_focus=(g_nwins>=2)?g_nwins-2:-1;
        }
        break;}
    case 73:{

        win_t *w=walloc("Pong",80,60,640,480);
        if(w){
            w->app_type=WAPP_PONG;
            drawpong_win(w,1);
            vbe_flip_rect(w->x,w->y,w->w,w->h);
            cmd_pong_gui(w->x,w->y,w->w,w->h);
            w->alive=0;
            g_focus=(g_nwins>=2)?g_nwins-2:-1;
        }
        break;}
    case 74:{

        win_t *w=walloc("Minecraft",80,60,800,600);
        if(w){
            w->app_type=WAPP_MINECRAFT;
            drawminecraft(w,1);
            vbe_flip_rect(w->x,w->y,w->w,w->h);
            cmd_minecraft_gui(w->x,w->y,w->w,w->h);
            w->alive=0;
            g_focus=(g_nwins>=2)?g_nwins-2:-1;
        }
        break;}
    case 23: // matrix old id - redirected
    case 24: // nyan old id
    case 25: // pipes old id
    case 26: // doomscroll old id
    case 27: // spinning old id
    case 28: // sl old id
    case 30:{win_t *w=walloc("Hex Calc",220,100,480,300);
        if(w){w->app_type=WAPP_HEXCALC;
        smact_run(w,"hexcalc 255");wwrite(w,"$ ");}break;}
    case 31:{win_t *w=walloc("Base64",120,80,600,340);
        if(w){w->app_type=WAPP_TEXT;
        _scpy(w->queued_cmd,"base64 ",INPUT_MAX);
        smact_run(w,"base64 Hello World");wwrite(w,"$ ");}break;}
    case 32:{win_t *w=walloc("Morse Code",160,80,680,400);
        if(w){w->app_type=WAPP_MORSE;smact_run(w,"morse stinger");wwrite(w,"$ ");}break;}
    case 33:{win_t *w=walloc("Image Viewer",100,80,600,400);
        if(w){w->app_type=WAPP_IMAGEVIEW;}break;}
    case 34:{win_t *w=walloc("Media Player",100,80,600,400);
        if(w){w->app_type=WAPP_MEDIAPLAYER;}break;}
    case 38:{win_t *w=walloc("FemboyGL Test",80,60,640,480);
        if(w){w->app_type=WAPP_FEMBOYGL_TEST;drawfemboygl_test(w,1);vbe_flip_rect(w->x,w->y,w->w,w->h);}break;}
    case 35:{win_t *w=walloc("Lolcat",80,60,760,500);
        if(w){w->app_type=WAPP_TEXT;
        _scpy(w->queued_cmd,"lolcat ",INPUT_MAX);
        smact_run(w,"lolcat Hello stinger");wwrite(w,"$ ");}break;}
    case 36:{win_t *w=walloc("Cowsay",130,90,500,360);
        if(w){w->app_type=WAPP_COWSAY;smact_run(w,"cowsay Moo!");wwrite(w,"$ ");}break;}
    case 37:{win_t *w=walloc("Rot13",130,90,500,300);
        if(w){w->app_type=WAPP_TEXT;
        _scpy(w->queued_cmd,"rot13 ",INPUT_MAX);
        smact_run(w,"rot13 Hello World");wwrite(w,"$ ");}break;}
    case 55:{win_t *w=walloc("Figlet Banner",160,80,680,400);
        if(w){w->app_type=WAPP_FIGLET;smact_run(w,"figlet stinger");wwrite(w,"$ ");}break;}
    case 40:{win_t *w=walloc("Network",100,60,700,440);
        if(w){w->app_type=WAPP_NETWORK;w->net_tab=0;
        smact_run(w,"ifconfig");wwrite(w,"$ ");}break;}
    case 44:{win_t *w=walloc("IRC",80,60,700,500);
        if(w){w->app_type=WAPP_IRC;wwrite(w,"IRC Client — type: irc <server>\nIRC> ");}break;}
    case 45:{win_t *w=walloc("GCC",80,60,760,500);
        if(w){w->app_type=WAPP_GCC;smact_run(w,"gcc");wwrite(w,"$ gcc ");}break;}
    case 46:{win_t *w=walloc("Wget",120,80,600,340);
        if(w){w->app_type=WAPP_TERMINAL;wwrite(w,"Wget — use: wget <url>\n$ ");}break;}
    case 47:{win_t *w=walloc("WiFi",120,80,500,300);
        if(w){w->app_type=WAPP_NETWORK;w->net_tab=3;
        smact_run(w,"wifi status");wwrite(w,"$ ");}break;}
    case 48:{win_t *w=walloc("SSH",100,60,680,480);
        if(w){w->app_type=WAPP_SSH;wwrite(w,"SSH Client — type: ssh user@host\nSSH> ");}break;}
    case 50:{win_t *w=walloc("Task Monitor",120,80,600,400);
        if(w){w->app_type=WAPP_TASKMON;w->net_tab=0;
        smact_run(w,"ps");wwrite(w,"$ ");}break;}
    case 51:{win_t *w=walloc("System Info",120,80,660,420);
        if(w){w->app_type=WAPP_DISKMON;w->net_tab=0;
        smact_run(w,"diskinfo");wwrite(w,"$ ");}break;}
    case 56:{win_t *w=walloc("Uptime",120,80,440,200);
        if(w){w->app_type=WAPP_TASKMON;w->net_tab=1;
        smact_run(w,"uptime");wwrite(w,"$ ");}break;}
    case 57:{win_t *w=walloc("Dmesg",80,60,760,500);
        if(w){w->app_type=WAPP_TASKMON;w->net_tab=2;
        smact_run(w,"dmesg");wwrite(w,"$ ");}break;}
    case 60:{extern void acpi_poweroff(void);acpi_poweroff();__asm__ volatile("cli;hlt");for(;;);}
    case 61:{extern void acpi_reboot(void);acpi_reboot();__asm__ volatile("cli;hlt");for(;;);}
    case 62:{
        g_in_yaoigui=0;
        vbe_set_autoflip(1);
        vbe_terminal_clear();
        extern void shell_run(void);
        shell_run();
        break;}
    default:break;
    }
}


static void doclk(int mx,int my,uint8_t btn){
    (void)btn;
    int sh=vbe_screen_height;


    if(g_focus>=0&&g_focus<g_nwins&&g_wins[g_focus].file_picker&&g_wins[g_focus].file_picker->active){
        file_picker_t *fp=g_wins[g_focus].file_picker;
        int fx=60,fy=40,fw=vbe_screen_width-120,fh=sh-80;

        if (pin(mx, my, fx-2, fy-2, fw+4, fh+4)) {
            int bbase=fx+fw-46;

            if(pin(mx,my,bbase,fy,46,TITLE_H)){fp->active=0;return;}


            int sdx=fx, sdw=150;
            int aby=fy+32;
            int ldx=sdx+sdw, ldy=aby+38, ldw=fw-sdw, ldh=fh-32-38-60;
            int row_h=24;
            if(pin(mx,my,ldx,ldy,ldw,ldh)){
                int clicked=(my-ldy)/row_h+fp->scroll;
                if(clicked>=0&&clicked<fp->entry_count){
                    if(clicked==fp->sel&&fp->is_dir[clicked])fp_navigate(fp,clicked,g_wins[g_focus].app_type);
                    else {
                        fp->sel=clicked;

                        if(!fp->is_dir[clicked]) {
                            _scpy(fp->input, fp->entries[clicked], 128);
                            fp->input_len = _slen(fp->input);
                        }
                    }
                }
                return;
            }


            int fty=fy+fh-60;
            int btnw=80, btnh=28;

            int obx=fx+fw-170, oby=fty+12;
            if(pin(mx,my,obx,oby,btnw,btnh)){
                if(fp->input_len>0){
                    fp->input[fp->input_len]=0;
                    _scpy(g_wins[g_focus].loaded_file,fp->input,256);
                    fp->result_confirmed=(fp->mode==FP_MODE_SAVE)?2:1;
                    fp->active=0;
                } else if(fp->sel>=0 && fp->sel<fp->entry_count && !fp->is_dir[fp->sel]) {
                    _scpy(g_wins[g_focus].loaded_file,fp->entries[fp->sel],256);
                    fp->result_confirmed=1;
                    fp->active=0;
                }
                return;
            }

            int cbx=fx+fw-85, cby=fty+12;
            if(pin(mx,my,cbx,cby,btnw,btnh)){fp->active=0;return;}

            return;
        }
    }

    if(g_smenu){
        int smy=sh-TBAR_H-SM_TOTAL_H;
        if(!pin(mx,my,0,smy,SMWIDTH,SM_TOTAL_H)){g_smenu=0;return;}
        if(pin(mx,my,6,smy+SM_TOTAL_H-80,SM_ICON_W-12,30)){smact(62);return;}
        if(pin(mx,my,14,smy+SM_TOTAL_H-44,24,24)){smact(60);return;}
        int cat_x=SM_ICON_W,app_x=SM_ICON_W+SM_CAT_W;
        if(pin(mx,my,cat_x,smy+SM_HDR_H,SM_CAT_W,SMCAT_COUNT*SM_ITEMH)){
            int idx=(my-(smy+SM_HDR_H))/SM_ITEMH;
            if(idx>=0&&idx<SMCAT_COUNT){g_sm_cat=idx;return;}
        }
        if(pin(mx,my,app_x,smy+SM_HDR_H,SM_APP_W,sm_app_counts[g_sm_cat]*SM_ITEMH)){
            int idx=(my-(smy+SM_HDR_H))/SM_ITEMH;
            if(idx>=0&&idx<sm_app_counts[g_sm_cat]){
                smact(sm_apps[g_sm_cat][idx].id);return;
            }
        }
        return;
    }
    if(my>=sh-TBAR_H){
        if(mx<START_W){g_smenu^=1;return;}
        int bx=START_W+8;
        for(int i=0;i<g_nwins;i++){
            if(!g_wins[i].alive)continue;
            if(pin(mx,my,bx,sh-TBAR_H+3,132,TBAR_H-6)){totop(i);return;}
            bx+=134;
        }
        return;
    }
    for(int i=g_nwins-1;i>=0;i--){
        win_t *wi=&g_wins[i];
        if(!wi->alive||!pin(mx,my,wi->x,wi->y,wi->w,wi->h))continue;


        if(i != g_nwins-1) {
            totop(i);
            g_focus = g_nwins-1;
            wi = &g_wins[g_focus];
        }

        int lmx = mx - wi->x;
        int lmy = my - wi->y;


        if(wi->app_iface){
            AppInterface *ai = (AppInterface*)wi->app_iface;
            if(ai->on_event){
                GUI_Event ge;
                ge.type = EVENT_MOUSE_CLICK;
                ge.mouse_pos.x = lmx;
                ge.mouse_pos.y = lmy;
                ai->on_event((struct Window*)wi, &ge, ai->app_data);
                return;
            }
        }

        if(pin(mx,my,wi->x+wi->w-WBTN_W,wi->y,WBTN_W,TITLE_H)){
            wi->alive=0;g_focus=(g_nwins>=2)?g_nwins-2:-1;return;
        }

        if(pin(mx,my,wi->x+wi->w-WBTN_W*2,wi->y,WBTN_W,TITLE_H)){

        }

        {
            int rh=win_resize_handle(wi,mx,my);

            int in_title=pin(mx,my,wi->x,wi->y,wi->w-WBTN_W*3,TITLE_H);
            if(rh>=0&&!in_title){
                wi->resize_handle=rh;
                wi->resize_ox=mx;wi->resize_oy=my;
                wi->resize_rx=wi->x;wi->resize_ry=wi->y;
                wi->resize_rw=wi->w;wi->resize_rh=wi->h;
                return;
            }
        }

        if(pin(mx,my,wi->x,wi->y,wi->w-WBTN_W*3,TITLE_H)){
            wi->drag=1;wi->dox=mx-wi->x;wi->doy=my-wi->y;
            return;
        }
        if(wi->app_type==WAPP_MEDIAPLAYER) {

            if(!wi->loaded_file[0] || pin(mx, my, wi->x + wi->w/2 - 50, wi->y + TITLE_H + 24 + (wi->h-TITLE_H-24-44)/2 - 50, 100, 100)) {
                if(!wi->file_picker) wi->file_picker = fp_create(FP_MODE_OPEN, "Open Media");
                else { wi->file_picker->mode = FP_MODE_OPEN; wi->file_picker->active = 1; }
                fp_populate(wi->file_picker, WAPP_MEDIAPLAYER);
            }

            int fy = wi->y + wi->h - 44;
            if(pin(mx, my, wi->x + wi->w/2 - 40, fy + 10, 80, 24)) {

                sound_stop();
            }
            return;
        }

        if(wi->app_type==WAPP_CALC){
            int bw=(wi->w-24)/4,bh=36;
            int dy=wi->y+TITLE_H+8+50;
            const char *btns[]={"7","8","9","+","4","5","6","-","1","2","3","*","0",".","=","/","C","(",")","%"};
            for(int bi=0;bi<20;bi++){
                int col=bi%4,row=bi/4;
                int bx=wi->x+8+col*bw,bby=dy+row*(bh+4);
                if(bby+bh>wi->y+wi->h-4)break;
                if(pin(mx,my,bx+2,bby,bw-4,bh)){
                    const char *lbl=btns[bi];
                    if(lbl[0]=='='){
                        wi->inp[wi->inlen]=0;
                        char cmd[64]="calc "; int ci2=5;
                        for(int i2=0;i2<wi->inlen&&ci2<62;i2++) cmd[ci2++]=wi->inp[i2];
                        cmd[ci2]=0;
                        wwrite(wi,"\n");
                        run_cmd(wi,cmd);
                        wi->inlen=0;
                    } else if(lbl[0]=='C'){wi->inlen=0;
                    } else {if(wi->inlen<INPUT_MAX)wi->inp[wi->inlen++]=lbl[0];}
                    break;
                }
            }
        }

        if(wi->app_type==WAPP_NANO){
            int rbx2 = wi->x+wi->w-80, rby2 = wi->y+TITLE_H+3;
            if(pin(mx,my,rbx2,rby2,70,18)){
                smact_run(wi, "save tmp.py");
                run_cmd(wi, "python tmp.py");
            }
        }

        if(wi->app_type==WAPP_FILEMANAGER){
            int ly=wi->y+TITLE_H+28;
            if(pin(mx,my,wi->x,ly,wi->w,wi->h-TITLE_H-28)){
                int clicked=(my-ly)/22+wi->fm_scroll;
                if(clicked>=0&&clicked<wi->fm_count){
                    if(clicked==wi->fm_sel&&wi->fm_entries[clicked].is_dir)fm_navigate(wi,clicked);
                    else wi->fm_sel=clicked;
                }
            }
        }

        if(wi->app_type==WAPP_SETTINGS){
            int sbw2=164;
            if(pin(mx,my,wi->x,wi->y+TITLE_H+30,sbw2,wi->h-TITLE_H-30)){
                int tab2=(my-(wi->y+TITLE_H+30))/36;
                if(tab2>=0&&tab2<8)wi->settings_tab=tab2;
            }

            if(wi->settings_tab==0 && (wi->settings_tab_extra%10000)>0){
                int by2 = wi->settings_tab_extra%10000;
                if(pin(mx,my,wi->x+sbw2+8,by2,112,28)){
                    run_cmd(wi,"dhcp");
                }
            }

            if(wi->settings_tab==2 && (wi->settings_tab_extra%10000)>0){
                int ry2 = wi->settings_tab_extra%10000;
                if(pin(mx,my,wi->x+sbw2+8,ry2,wi->w-sbw2-16,34)){
                    sfs_node_t *_etc=sfs_resolve(sfs_root(),"/etc");
                    if(!_etc)_etc=sfs_mkdir(sfs_root(),"etc");
                    if(_etc){
                        sfs_node_t *_f=sfs_find_child(_etc,"gui_autostart");
                        int cur=1;
                        if(_f&&_f->data&&_f->size>0)cur=(((char*)_f->data)[0]!='0');
                        const char *val=cur?"0":"1";
                        sfs_write_file(_etc,"gui_autostart",(const uint8_t*)val,1);
                    }
                }
            }
        }

        if(wi->app_type==WAPP_BROWSER){
            int uby=wi->y+TITLE_H;
            if(pin(mx,my,wi->x+wi->w-56,uby+3,50,22))br_fetch(wi);
        }

        if(wi->app_type==WAPP_NETWORK){
            int taby=wi->y+TITLE_H;
            int ntab_count=6;
            int tw=wi->w/ntab_count;
            if(pin(mx,my,wi->x,taby,wi->w,24)){
                int idx=(mx-wi->x)/tw;
                if(idx>=0&&idx<ntab_count){
                    wi->net_tab=idx;
                    const char *cmds[]={"dhcp","ifconfig","ping 8.8.8.8","wifi status","nslookup google.com","wget"};
                    twin_clear(wi);
                    wwrite(wi,"$ ");wwrite(wi,cmds[idx]);wwrite(wi,"\n");
                    run_cmd(wi,cmds[idx]);
                }
            }
        }

        if(wi->app_type==WAPP_TASKMON){
            int taby=wi->y+TITLE_H;
            int ttab_count=4;
            int tw=wi->w/ttab_count;
            if(pin(mx,my,wi->x,taby,wi->w,24)){
                int idx=(mx-wi->x)/tw;
                if(idx>=0&&idx<ttab_count){
                    wi->net_tab=idx;
                    const char *cmds[]={"ps","uptime","dmesg","acpi"};
                    twin_clear(wi);
                    wwrite(wi,"$ ");wwrite(wi,cmds[idx]);wwrite(wi,"\n");
                    run_cmd(wi,cmds[idx]);
                }
            }
        }

        if(wi->app_type==WAPP_DISKMON){
            int taby=wi->y+TITLE_H;
            int dtab_count=6;
            int tw=wi->w/dtab_count;
            if(pin(mx,my,wi->x,taby,wi->w,24)){
                int idx=(mx-wi->x)/tw;
                if(idx>=0&&idx<dtab_count){
                    wi->net_tab=idx;
                    const char *cmds[]={"diskinfo","meminfo","lscpu","smpinfo","df","du"};
                    twin_clear(wi);
                    wwrite(wi,"$ ");wwrite(wi,cmds[idx]);wwrite(wi,"\n");
                    run_cmd(wi,cmds[idx]);
                }
            }
        }

        if(wi->app_type==WAPP_DRAW&&wi->draw){
            int tby=wi->y+TITLE_H+28;
            int cw2=wi->w-2,ch2=wi->h-TITLE_H-28-2;
            int csx=wi->x+1,csy=tby;
            int pw=cw2/DRAW_W,ph=ch2/DRAW_H;
            if(pw<1)pw=1;if(ph<1)ph=1;

            if(pin(mx,my,wi->x+6,wi->y+TITLE_H+6,56,18))wi->draw->tool=0;
            if(pin(mx,my,wi->x+70,wi->y+TITLE_H+6,56,18))wi->draw->tool=1;

            uint32_t pal[]={C_WHITE,C_RED,C_GREEN,C_CYAN,C_YELLOW,C_ACCENT,C(255,128,0),C(128,0,255)};
            for(int pi=0;pi<8;pi++){
                int px2=wi->x+140+pi*22;
                if(pin(mx,my,px2,wi->y+TITLE_H+4,18,18)){wi->draw->fg_color=pal[pi];}
            }

            if(pin(mx,my,csx,csy,cw2,ch2)){
                wi->draw->drawing=1;
                int cx2=(mx-csx)/pw;
                int cy2=(my-csy)/ph;
                if(cx2>=0&&cx2<DRAW_W&&cy2>=0&&cy2<DRAW_H){
                    wi->draw->pixels[cy2][cx2]=wi->draw->tool==0?wi->draw->fg_color:C(30,30,40);
                }
            }
        }
        return;
    }
}

static void dorel(void){
    for(int i=0;i<g_nwins;i++){
        g_wins[i].drag=0;
        g_wins[i].resize_handle=-1;
        if(g_wins[i].draw)g_wins[i].draw->drawing=0;
    }
}

static void domov(int mx,int my){
    int sh=vbe_screen_height,sw=vbe_screen_width;
    g_stho=(my>=sh-TBAR_H&&mx<START_W);
    smenu_hittest(mx,my);

    for(int i=0;i<g_nwins;i++){
        win_t *wi=&g_wins[i];
        if(wi->alive&&wi->app_type==WAPP_DRAW&&wi->draw&&wi->draw->drawing){
            int tby=wi->y+TITLE_H+28;
            int cw2=wi->w-2,ch2=wi->h-TITLE_H-28-2;
            int csx=wi->x+1,csy=tby;
            int pw=cw2/DRAW_W,ph=ch2/DRAW_H;
            if(pw<1)pw=1;if(ph<1)ph=1;
            int cx2=(mx-csx)/pw;
            int cy2=(my-csy)/ph;
            if(cx2>=0&&cx2<DRAW_W&&cy2>=0&&cy2<DRAW_H)
                wi->draw->pixels[cy2][cx2]=wi->draw->tool==0?wi->draw->fg_color:C(30,30,40);
        }
    }

    if(g_focus>=0&&g_focus<g_nwins&&g_wins[g_focus].drag){
        win_t *wi=&g_wins[g_focus];
        wi->x=mx-wi->dox;wi->y=my-wi->doy;
        if(wi->x<0) wi->x=0;
        if(wi->y<0) wi->y=0;
        if(wi->x+wi->w>sw)wi->x=sw-wi->w;
        if(wi->y+wi->h>sh-TBAR_H)wi->y=sh-TBAR_H-wi->h;
        wi->dirty=1;
    }

    if(g_focus>=0&&g_focus<g_nwins){
        win_t *wi=&g_wins[g_focus];
        int h=wi->resize_handle;
        if(wi->alive&&h>=0){
            int dx=mx-wi->resize_ox,dy=my-wi->resize_oy;
            int nx=wi->resize_rx,ny=wi->resize_ry;
            int nw=wi->resize_rw,nh=wi->resize_rh;

            if(h==0){nx+=dx;ny+=dy;nw-=dx;nh-=dy;}
            else if(h==1){ny+=dy;nh-=dy;}
            else if(h==2){ny+=dy;nw+=dx;nh-=dy;}
            else if(h==3){nx+=dx;nw-=dx;}
            else if(h==4){nw+=dx;}
            else if(h==5){nx+=dx;nw-=dx;nh+=dy;}
            else if(h==6){nh+=dy;}
            else if(h==7){nw+=dx;nh+=dy;}

            if(nw<WIN_MIN_W){if(h==0||h==3||h==5)nx=wi->resize_rx+(wi->resize_rw-WIN_MIN_W);nw=WIN_MIN_W;}
            if(nh<WIN_MIN_H){if(h==0||h==1||h==2)ny=wi->resize_ry+(wi->resize_rh-WIN_MIN_H);nh=WIN_MIN_H;}

            if(nx<0) nx=0;
            if(ny<0) ny=0;
            if(nx+nw>sw)nw=sw-nx;
            if(ny+nh>sh-TBAR_H)nh=sh-TBAR_H-ny;
            wi->x=nx;wi->y=ny;wi->w=nw;wi->h=nh;
            wi->dirty=1;
        }
    }
}


static void drawtbar(void){
    int sw=vbe_screen_width,sh=vbe_screen_height,ty=sh-TBAR_H;
    dr(0,ty,sw,TBAR_H,C_TBAR);
    dr(0,ty,sw,1,C_TBAR_TOP);
    uint32_t sbg=g_stho?C(55,55,55):g_smenu?C(70,70,70):C_TBAR;
    dr(0,ty,START_W,TBAR_H,sbg);
    dricon((START_W-16)/2,(ty+(TBAR_H-16)/2),&ICO_SCORP);
    if(g_smenu)dr(4,ty+TBAR_H-3,START_W-8,3,C_ACCENT);
    int bx=START_W+8;
    for(int i=0;i<g_nwins;i++){
        if(!g_wins[i].alive)continue;
        int active=(i==g_focus);
        uint32_t bc=active?C_TBNA:C_TBNI;
        int bw=132;
        dr(bx,ty+3,bw,TBAR_H-6,bc);
        const icon_t *tico;
        switch(g_wins[i].app_type){
        case WAPP_FILEMANAGER: tico=&ICO_FOLDER;   break;
        case WAPP_BROWSER:     tico=&ICO_BROWSER;  break;
        case WAPP_SETTINGS:    tico=&ICO_SETTINGS; break;
        case WAPP_SYSINFO:     tico=&ICO_SYSINFO;  break;
        case WAPP_CALC:        tico=&ICO_TOOLS;    break;
        case WAPP_HEXCALC:     tico=&ICO_TOOLS;    break;
        case WAPP_NANO:        tico=&ICO_NANO;     break;
        case WAPP_MORSE:       tico=&ICO_NET;      break;
        case WAPP_ASCII:       tico=&ICO_TOOLS;    break;
        case WAPP_FIGLET:      tico=&ICO_TOOLS;    break;
        case WAPP_COWSAY:      tico=&ICO_TOOLS;    break;
        case WAPP_DRAW:        tico=&ICO_DRAW;     break;
        case WAPP_GCC:         tico=&ICO_GCC;      break;
        case WAPP_NETWORK:     tico=&ICO_NET;      break;
        case WAPP_TASKMON:     tico=&ICO_SYSINFO;  break;
        case WAPP_DISKMON:     tico=&ICO_SYSINFO;  break;
        case WAPP_TEXT:        tico=&ICO_TOOLS;    break;
        case WAPP_IRC:         tico=&ICO_NET;      break;
        case WAPP_SSH:         tico=&ICO_NET;      break;
        case WAPP_GAMES:       tico=&ICO_GAME;     break;
        default:               tico=&ICO_TERM;     break;
        }

        if(g_wins[i].busy) dr(bx+bw-8,ty+4,6,6,C_YELLOW);
        dricon(bx+6,ty+(TBAR_H-16)/2,tico);
        char tmp[12];int tl=_slen(g_wins[i].title);
        int mc=10;if(tl<mc)mc=tl;
        int ci=0;for(;ci<mc;ci++)tmp[ci]=g_wins[i].title[ci];tmp[ci]=0;
        ds(bx+26,ty+(TBAR_H-FH)/2,tmp,active?C_WHITE:C_LGREY,bc);
        if(active)dr(bx+4,ty+TBAR_H-4,bw-8,3,C_ACCENT);
        bx+=bw+2;
    }
    rtc_t t=rtc_now();
    char h2[3],m2[3];_pad2(h2,t.h);_pad2(m2,t.m);
    char clk[6];clk[0]=h2[0];clk[1]=h2[1];clk[2]=':';clk[3]=m2[0];clk[4]=m2[1];clk[5]=0;
    int clkw=_slen(clk)*FW;
    int clkx=sw-clkw-16;
    dr(sw-clkw-32,ty,clkw+36,TBAR_H,C_TBAR);
    ds(clkx,ty+(TBAR_H-FH)/2,clk,C_LGREY,C_TBAR);
}


static volatile int32_t g_apdx=0,g_apdy=0;
static volatile uint8_t g_apbtn=0;
static volatile int g_apready=0;


static int g_vol_timer = 0;

static void draw_volume_overlay(void) {
    if (g_vol_timer <= 0) return;
    int sw = vbe_screen_width, sh = vbe_screen_height;
    int vw = 48, vh = 160;
    int vx = 30, vy = 60;


    dr(vx, vy, vw, vh, C(25, 25, 25));
    dbdr(vx, vy, vw, vh, C(60, 60, 60));

    int vol = sound_get_volume();
    int bar_h = (vol * (vh - 40)) / 100;


    dr(vx + 16, vy + vh - 10 - bar_h, 16, bar_h, C_ACCENT);
    dbdr(vx + 16, vy + 10, 16, vh - 40, C(80, 80, 80));


    char vstr[4]; _itoa(vstr, vol);
    int tw = _slen(vstr) * FW;
    ds(vx + (vw - tw) / 2, vy + vh - 25, vstr, C_WHITE, C(25, 25, 25));
}




void __attribute__((noreturn)) yaoigui_run(void){
    g_in_yaoigui=1;
    vbe_set_autoflip(0);
    int sw=vbe_screen_width,sh=vbe_screen_height;
    if(sw<=0) sw=640;
    if(sh<=0) sh=480;


    gui_load_wallpaper("default.ppm");
    mouse_x=sw/2;mouse_y=sh/2;


    draw_background();
    vbe_flip();



    int ap_online=0;
    win_t *term=walloc("Terminal",60,50,780,500);
    if(term){
        term->app_type=WAPP_TERMINAL;
        char dbg[128];
        _itoa(dbg,g_cpu_count);
        wwrite(term,"stinger YaoiGUI v4.0 — type commands below\n");
        wwrite(term,"CPUs: ");wwrite(term,dbg);
        for(int _i=0;_i<g_cpu_count;_i++)
            if(!g_cpus[_i].is_bsp&&g_cpus[_i].online){ap_online=1;break;}
        wwrite(term,ap_online?" (AP online - async mode)":"-BSP only (sync mode)");
        wwrite(term,"\n$ ");
    }

    uint32_t last=_ticks(),last_sec=0;
    uint8_t lastbtn=0;
    int dirty=1,mouse_moved=0;
    int last_mx=mouse_x,last_my=mouse_y;

    for(;;){

        mouse_poll_backends();
        mouse_event_t ev;
        int max_m=64;
        while(mouse_poll(&ev) && max_m--){
            mouse_x+=ev.dx;mouse_y-=ev.dy;
            if(mouse_x<0) { mouse_x=0; }
            if(mouse_x>=sw) { mouse_x=sw-1; }
            if(mouse_y<0) { mouse_y=0; }
            if(mouse_y>=sh) { mouse_y=sh-1; }
            if((ev.buttons&MOUSE_BTN_LEFT)&&!(lastbtn&MOUSE_BTN_LEFT)){doclk(mouse_x,mouse_y,ev.buttons);dirty=1;}
            if(!(ev.buttons&MOUSE_BTN_LEFT)&&(lastbtn&MOUSE_BTN_LEFT)){dorel();dirty=1;}
            if(ev.dz!=0){doscroll(-ev.dz);dirty=1;}
            domov(mouse_x,mouse_y);
            if(mouse_x!=last_mx||mouse_y!=last_my) mouse_moved=1;
            for(int _j=0;_j<g_nwins;_j++) if(g_wins[_j].drag || g_wins[_j].resize_handle>=0) {dirty=1; break;}
            lastbtn=ev.buttons;
        }

        while(keyboard_keyready()){
            char k=keyboard_poll();
            if(k){
                if(k=='\x02'){g_smenu^=1;dirty=1;}
                else if(k=='\x11'){ sound_toggle_mute(); g_vol_timer=120; dirty=1; }
                else if(k=='\x12'){ sound_set_volume(sound_get_volume() - 10); g_vol_timer=120; dirty=1; }
                else if(k=='\x13'){ sound_set_volume(sound_get_volume() + 10); g_vol_timer=120; dirty=1; }
                else {dokey(k); dirty=1;}
            }
        }

        if(g_vol_timer > 0) { g_vol_timer--; dirty=1; }


        for(int i=0; i<g_nwins; i++) {
            win_t *wi = &g_wins[i];
            if(wi->file_picker && wi->file_picker->result_confirmed) {
                if(wi->app_type == WAPP_MEDIAPLAYER && wi->file_picker->result_confirmed >= 1) {

                    sfs_node_t *dir = sfs_resolve(sfs_root(), wi->file_picker->path);
                    if(dir) {
                        sfs_node_t *file = sfs_find_child(dir, wi->loaded_file);
                        if(file) {
                            wi->busy = 1;

                            drawwin(wi, i == g_focus);
                            vbe_flip();
                            extern int media_play(sfs_node_t *file, const char *filename);
                            media_play(file, wi->loaded_file);
                            wi->busy = 0;
                            dirty = 1;
                        }
                    }
                }
                wi->file_picker->result_confirmed = 0;
            }
        }


        uint32_t now=_ticks();
        if((uint32_t)(now-last)<16){ __asm__ volatile("pause"); continue; }
        last=now;

        uint32_t cur_sec=now/1000;
        int clock_dirty = (cur_sec!=last_sec);
        if(clock_dirty) last_sec=cur_sec;


        int win_dirty = 0;
        for(int i=0; i<g_nwins; i++) if(g_wins[i].dirty || g_wins[i].busy || g_wins[i].app_type == WAPP_FEMBOYGL_TEST) { win_dirty=1; break; }

        if(dirty || win_dirty || clock_dirty || mouse_moved){
            if(dirty) draw_background();
            for(int i=0;i<g_nwins;i++) {
                drawwin(&g_wins[i], i==g_focus);
                g_wins[i].dirty = 0;
            }
            if(g_smenu) drawsmenu();
            drawtbar();
            if(g_focus>=0 && g_wins[g_focus].file_picker && g_wins[g_focus].file_picker->active) fp_draw(g_wins[g_focus].file_picker, 1);
            draw_volume_overlay();
            cur_draw_hw(mouse_x, mouse_y);
            vbe_flip();
            dirty=0; mouse_moved=0;
            last_mx=mouse_x; last_my=mouse_y;
        }
        sfs_persist_flush();
    }
}


int vbe_get_width(void) { extern int vbe_screen_width; return vbe_screen_width; }
int vbe_get_height(void) { extern int vbe_screen_height; return vbe_screen_height; }

void fgl_fill_rect(int x, int y, int w, int h, uint32_t color) {
    vbe_fill_rect(x, y, w, h, color);
}

void fgl_draw_string(int x, int y, const char* s, uint32_t color, uint32_t bg) {
    (void)bg;
    while(*s) {
        vbe_draw_char_mask(x, y, *s++, color);
        x += 8;
    }
}

void fgl_draw_rect_border(int x, int y, int w, int h, uint32_t color) {
    vbe_fill_rect(x, y, w, 1, color);
    vbe_fill_rect(x, y + h - 1, w, 1, color);
    vbe_fill_rect(x, y, 1, h, color);
    vbe_fill_rect(x + w - 1, y, 1, h, color);
}


size_t strlen(const char* s) {
    size_t len = 0;
    while(s[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while(n > 0 && *src) {
        *d++ = *src++;
        n--;
    }
    while(n > 0) {
        *d++ = '\0';
        n--;
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}





