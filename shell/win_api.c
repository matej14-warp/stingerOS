








static void *wa_memset(void *d, int c, size_t n){
    uint8_t *p = d;
    while(n--) *p++ = (uint8_t)c;
    return d;
}
static int wa_slen(const char *s){ int n=0; while(*s++) n++; return n; }
















typedef struct {
    wapi_event_t events[WA_EV_MAX];
    int head, tail, count;
} wa_evq_t;

static void evq_push(wa_evq_t *q, const wapi_event_t *ev){
    if(q->count >= WA_EV_MAX) return;
    q->events[q->tail] = *ev;
    q->tail = (q->tail + 1) % WA_EV_MAX;
    q->count++;
}

static int evq_pop(wa_evq_t *q, wapi_event_t *out){
    if(q->count <= 0) return 0;
    *out = q->events[q->head];
    q->head = (q->head + 1) % WA_EV_MAX;
    q->count--;
    return 1;
}


typedef struct {
    int alive;
    int x, y, w, h;
    int drag, dox, doy;
    int focused;
    char title[48];
    uint32_t bg;
    wa_evq_t evq;

} wa_win_t;

static wa_win_t g_wa_wins[WAPI_MAX_WINS];
static int      g_wa_count   = 0;
static int      g_wa_focus   = -1;
static int      g_wa_inited  = 0;


void wapi_init(void){
    if(g_wa_inited) return;
    wa_memset(g_wa_wins, 0, sizeof(g_wa_wins));
    g_wa_count  = 0;
    g_wa_focus  = -1;
    g_wa_inited = 1;
}


wapi_handle_t wapi_create(const char *title, int x, int y, int w, int h){
    wapi_init();
    if(g_wa_count >= WAPI_MAX_WINS) return WAPI_INVALID;


    int slot = -1;
    for(int i = 0; i < WAPI_MAX_WINS; i++){
        if(!g_wa_wins[i].alive){ slot = i; break; }
    }
    if(slot < 0) return WAPI_INVALID;

    wa_win_t *wi = &g_wa_wins[slot];
    wa_memset(wi, 0, sizeof(*wi));
    wi->alive   = 1;
    wi->x       = x;
    wi->y       = y;
    wi->w       = w < 80  ? 80  : w;
    wi->h       = h < 60  ? 60  : h;
    wi->bg      = WA_DEFAULT_BG;

    int ti = 0;
    while(title[ti] && ti < 47){ wi->title[ti] = title[ti]; ti++; }
    wi->title[ti] = 0;

    g_wa_focus = slot;
    g_wa_count++;


    wapi_flush((wapi_handle_t)slot);
    return (wapi_handle_t)slot;
}

void wapi_destroy(wapi_handle_t h){
    if(h < 0 || h >= WAPI_MAX_WINS) return;
    wa_win_t *wi = &g_wa_wins[h];
    if(!wi->alive) return;


    vbe_fill_rect(wi->x, wi->y, wi->w, wi->h, 0x171717);
    vbe_flip_rect(wi->x, wi->y, wi->w, wi->h);

    wi->alive = 0;
    g_wa_count--;
    if(g_wa_focus == h){
        g_wa_focus = -1;

        for(int i = WAPI_MAX_WINS - 1; i >= 0; i--){
            if(g_wa_wins[i].alive){ g_wa_focus = i; break; }
        }
    }
}

int wapi_alive(wapi_handle_t h){
    if(h < 0 || h >= WAPI_MAX_WINS) return 0;
    return g_wa_wins[h].alive;
}


int wapi_width(wapi_handle_t h){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return 0;
    return g_wa_wins[h].w;
}
int wapi_height(wapi_handle_t h){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return 0;
    return g_wa_wins[h].h - WA_TITLE_H;
}


static inline int wa_cx(wa_win_t *wi){ return wi->x; }
static inline int wa_cy(wa_win_t *wi){ return wi->y + WA_TITLE_H; }
static inline int wa_cw(wa_win_t *wi){ return wi->w; }
static inline int wa_ch(wa_win_t *wi){ return wi->h - WA_TITLE_H; }

void wapi_clear(wapi_handle_t h, uint32_t colour){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];
    wi->bg = colour;
    vbe_fill_rect(wa_cx(wi), wa_cy(wi), wa_cw(wi), wa_ch(wi), colour);
}

void wapi_fill_rect(wapi_handle_t h, int x, int y, int w, int ht, uint32_t colour){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];
    vbe_fill_rect(wa_cx(wi) + x, wa_cy(wi) + y, w, ht, colour);
}

void wapi_draw_char(wapi_handle_t h, int x, int y, char c, uint32_t fg){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];
    vbe_draw_char(wa_cx(wi) + x, wa_cy(wi) + y, c, fg, wi->bg);
}

void wapi_draw_text(wapi_handle_t h, int x, int y, const char *s, uint32_t fg){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];
    int bx = wa_cx(wi) + x;
    int by = wa_cy(wi) + y;
    while(*s){
        vbe_draw_char(bx, by, *s++, fg, wi->bg);
        bx += WA_FW;
    }
}

void wapi_put_pixel(wapi_handle_t h, int x, int y, uint32_t colour){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];
    vbe_put_pixel(wa_cx(wi) + x, wa_cy(wi) + y, colour);
}


static void wa_draw_titlebar(wa_win_t *wi, int focused){
    int x = wi->x, y = wi->y, w = wi->w;
    uint32_t tbg = focused ? WA_TBAR_A : WA_TBAR_I;
    vbe_fill_rect(x, y, w, WA_TITLE_H, tbg);


    int tx = x + 10;
    int ty = y + (WA_TITLE_H - WA_FH) / 2;
    const char *t = wi->title;
    while(*t){
        vbe_draw_char(tx, ty, *t++, focused ? 0xFFFFFF : 0xAAAAAA, tbg);
        tx += WA_FW;
    }


    int cbx = x + w - WA_BTN_W;
    vbe_fill_rect(cbx, y, WA_BTN_W, WA_TITLE_H, focused ? WA_CLOSE_C : tbg);
    vbe_draw_char(cbx + (WA_BTN_W - WA_FW) / 2,
                  y + (WA_TITLE_H - WA_FH) / 2,
                  'X', 0xFFFFFF, focused ? WA_CLOSE_C : tbg);


    if(focused)
        vbe_fill_rect(x, y + WA_TITLE_H - 2, w - WA_BTN_W, 2, 0x0078D7);


    uint32_t border = focused ? WA_BORD_A : WA_BORD_I;

    vbe_fill_rect(x, y, w, 1, border);

    vbe_fill_rect(x, y + wi->h - 1, w, 1, border);

    vbe_fill_rect(x, y, 1, wi->h, border);

    vbe_fill_rect(x + w - 1, y, 1, wi->h, border);
}


void wapi_flush(wapi_handle_t h){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];
    int focused = (g_wa_focus == h);
    wa_draw_titlebar(wi, focused);
    vbe_flip_rect(wi->x, wi->y, wi->w, wi->h);
}


void wapi_push_key(wapi_handle_t h, char k){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wapi_event_t ev = {0};
    ev.type = WAPI_EV_KEY;
    ev.key  = k;
    evq_push(&g_wa_wins[h].evq, &ev);
}

void wapi_push_click(wapi_handle_t h, int mx, int my, int btn){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wa_win_t *wi = &g_wa_wins[h];

    wapi_event_t ev = {0};
    ev.type = WAPI_EV_CLICK;
    ev.mx   = mx - wa_cx(wi);
    ev.my   = my - wa_cy(wi);
    ev.btn  = btn;
    evq_push(&g_wa_wins[h].evq, &ev);
}

void wapi_push_close(wapi_handle_t h){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return;
    wapi_event_t ev = {0};
    ev.type = WAPI_EV_CLOSE;
    evq_push(&g_wa_wins[h].evq, &ev);
}

int wapi_poll(wapi_handle_t h, wapi_event_t *ev){
    if(h < 0 || h >= WAPI_MAX_WINS || !g_wa_wins[h].alive) return 0;
    return evq_pop(&g_wa_wins[h].evq, ev);
}




void wapi_redraw_all(void){
    for(int i = 0; i < WAPI_MAX_WINS; i++){
        if(!g_wa_wins[i].alive) continue;
        int focused = (g_wa_focus == i);
        wa_draw_titlebar(&g_wa_wins[i], focused);
    }
}


void wapi_handle_click(int mx, int my, int btn){
    (void)btn;
    for(int i = WAPI_MAX_WINS - 1; i >= 0; i--){
        wa_win_t *wi = &g_wa_wins[i];
        if(!wi->alive) continue;
        if(mx < wi->x || mx >= wi->x + wi->w) continue;
        if(my < wi->y || my >= wi->y + wi->h) continue;


        g_wa_focus = i;


        if(mx >= wi->x + wi->w - WA_BTN_W && my < wi->y + WA_TITLE_H){
            wapi_push_close(i);


            return;
        }


        if(my < wi->y + WA_TITLE_H){
            wi->drag = 1;
            wi->dox  = mx - wi->x;
            wi->doy  = my - wi->y;
            return;
        }


        wapi_push_click(i, mx, my, btn);
        return;
    }
}


void wapi_handle_mousemove(int mx, int my){
    int sw = vbe_screen_width;
    int sh = vbe_screen_height;
    for(int i = 0; i < WAPI_MAX_WINS; i++){
        wa_win_t *wi = &g_wa_wins[i];
        if(!wi->alive || !wi->drag) continue;
        wi->x = mx - wi->dox;
        wi->y = my - wi->doy;
        if(wi->x < 0) wi->x = 0;
        if(wi->y < 0) wi->y = 0;
        if(wi->x + wi->w > sw) wi->x = sw - wi->w;
        if(wi->y + wi->h > sh - 44) wi->y = sh - 44 - wi->h;
    }
}

void wapi_handle_mouseup(void){
    for(int i = 0; i < WAPI_MAX_WINS; i++)
        g_wa_wins[i].drag = 0;
}


void wapi_handle_key(char k){
    if(g_wa_focus < 0) return;
    wapi_push_key((wapi_handle_t)g_wa_focus, k);
}




