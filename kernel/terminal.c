








static spinlock_t terminal_lock = SPINLOCK_INIT;



static volatile uint16_t * const VGA_BUF = (volatile uint16_t *)0xB8000;

static size_t  t_col   = 0;
static size_t  t_row   = 0;
static uint8_t t_color = 0x07;
static void (*t_hook)(char) = NULL;
static void (*t_clear_hook)(void) = NULL;

void terminal_init(void) {
    terminal_clear();
}

void terminal_set_hook(void (*hook)(char)) { t_hook = hook; }
void terminal_set_clear_hook(void (*fn)(void)) { t_clear_hook = fn; }

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

void terminal_setcolor(uint8_t color) { t_color = color; }

static void vga_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUF[y * VGA_WIDTH + x] = VGA_BUF[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_BUF[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', t_color);
    }
}

void terminal_clear(void) {
    spin_lock(&terminal_lock);
    if (t_clear_hook) t_clear_hook();
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUF[y * VGA_WIDTH + x] = vga_entry(' ', t_color);
        }
    }
    t_col = 0; t_row = 0;
    spin_unlock(&terminal_lock);
}


static int g_vga_esc_state = 0;
static int g_vga_esc_params[4];
static int g_vga_esc_pcount = 0;
static int g_vga_save_x=0, g_vga_save_y=0;

static void vga_ansi_dispatch(char c) {
    switch (c) {
        case 'm': {
            for (int i=0; i<g_vga_esc_pcount; i++) {
                int p = g_vga_esc_params[i];
                if (p == 0) t_color = 0x07;
                else if (p == 1) t_color |= 0x08;
                else if (p >= 30 && p <= 37) {
                    static const uint8_t map[] = {0,4,2,6,1,5,3,7};
                    t_color = (t_color & 0xF0) | map[p-30];
                }
            }
            break;
        }
        case 'H': case 'f':
            t_row = (g_vga_esc_params[0] ? g_vga_esc_params[0]-1 : 0);
            t_col = (g_vga_esc_pcount > 1 && g_vga_esc_params[1] ? g_vga_esc_params[1]-1 : 0);
            if (t_row >= VGA_HEIGHT) t_row = VGA_HEIGHT-1;
            if (t_col >= VGA_WIDTH)  t_col = VGA_WIDTH-1;
            break;
        case 's': g_vga_save_x = (int)t_col; g_vga_save_y = (int)t_row; break;
        case 'u': t_col = (size_t)g_vga_save_x; t_row = (size_t)g_vga_save_y; break;
    }
}

void terminal_putchar(char c) {
    spin_lock(&terminal_lock);
    if (t_hook) t_hook(c);
    if (vbe_active()) {
        vbe_terminal_putchar(c);
        spin_unlock(&terminal_lock);
        return;
    }


    if (g_vga_esc_state == 1) {
        if (c == '[') { g_vga_esc_state = 2; g_vga_esc_pcount = 0; g_vga_esc_params[0] = 0; spin_unlock(&terminal_lock); return; }
        g_vga_esc_state = 0;
    } else if (g_vga_esc_state == 2) {
        if (c >= '0' && c <= '9') {
            g_vga_esc_params[g_vga_esc_pcount] = g_vga_esc_params[g_vga_esc_pcount] * 10 + (c - '0');
            spin_unlock(&terminal_lock);
            return;
        } else if (c == ';') {
            if (g_vga_esc_pcount < 3) { g_vga_esc_pcount++; g_vga_esc_params[g_vga_esc_pcount] = 0; }
            spin_unlock(&terminal_lock);
            return;
        } else {
            g_vga_esc_pcount++;
            vga_ansi_dispatch(c);
            g_vga_esc_state = 0;
            spin_unlock(&terminal_lock);
            return;
        }
    }
    if (c == '\x1b') { g_vga_esc_state = 1; spin_unlock(&terminal_lock); return; }

    if (c == '\n') {
        t_col = 0;
        if (++t_row == VGA_HEIGHT) { vga_scroll(); t_row = VGA_HEIGHT-1; }
        spin_unlock(&terminal_lock);
        return;
    }
    if (c == '\r') { t_col = 0; spin_unlock(&terminal_lock); return; }
    if (c == '\b') {
        if (t_col > 0) { t_col--; VGA_BUF[t_row*VGA_WIDTH+t_col] = vga_entry(' ', t_color); }
        spin_unlock(&terminal_lock);
        return;
    }
    if (c == '\t') {
        t_col = (t_col + 8) & ~(size_t)7;
        if (t_col >= VGA_WIDTH) { t_col = 0; if (++t_row == VGA_HEIGHT) { vga_scroll(); t_row=VGA_HEIGHT-1; } }
        spin_unlock(&terminal_lock);
        return;
    }
    VGA_BUF[t_row*VGA_WIDTH+t_col] = vga_entry(c, t_color);
    if (++t_col == VGA_WIDTH) {
        t_col = 0;
        if (++t_row == VGA_HEIGHT) { vga_scroll(); t_row = VGA_HEIGHT-1; }
    }
    spin_unlock(&terminal_lock);
}

void terminal_write(const char *data, size_t size) {
    for (size_t i=0;i<size;i++) terminal_putchar(data[i]);
}

void terminal_writestring(const char *data) {
    while (*data) terminal_putchar(*data++);
}


static void put_uint(unsigned long long v, int base, int upper, int width, int zero_pad) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char buf[64]; int i = 0;
    if (!v) buf[i++] = '0';
    while (v) { buf[i++] = digits[v % (unsigned long long)base]; v /= (unsigned long long)base; }
    int pad = width - i;
    while (pad-- > 0) terminal_putchar(zero_pad ? '0' : ' ');
    while (i > 0) terminal_putchar(buf[--i]);
}

void terminal_printf(const char *fmt, ...) {
    spin_lock(&terminal_lock);
    va_list ap; va_start(ap, fmt);
    while (*fmt) {
        if (*fmt != '%') { terminal_putchar(*fmt++); continue; }
        fmt++;
        int zero_pad = 0, width = 0;
        if (*fmt == '0') { zero_pad = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width*10 + (*fmt++ - '0'); }

        int lng = 0;
        if (*fmt == 'l') { lng++; fmt++; if (*fmt == 'l') { lng++; fmt++; } }
        switch (*fmt++) {
        case 'd': {
            long long v = lng>=2 ? va_arg(ap,long long) : lng ? va_arg(ap,long) : va_arg(ap,int);
            if (v < 0) { terminal_putchar('-'); v = -v; }
            put_uint((unsigned long long)v, 10, 0, width, zero_pad);
            break; }
        case 'u': {
            unsigned long long v = lng>=2 ? va_arg(ap,unsigned long long) : lng ? va_arg(ap,unsigned long) : va_arg(ap,unsigned);
            put_uint(v, 10, 0, width, zero_pad); break; }
        case 'x': {
            unsigned long long v = lng>=2 ? va_arg(ap,unsigned long long) : lng ? va_arg(ap,unsigned long) : va_arg(ap,unsigned);
            put_uint(v, 16, 0, width, zero_pad); break; }
        case 'X': {
            unsigned long long v = lng>=2 ? va_arg(ap,unsigned long long) : lng ? va_arg(ap,unsigned long) : va_arg(ap,unsigned);
            put_uint(v, 16, 1, width, zero_pad); break; }
        case 'p': {
            unsigned long v = (unsigned long)(uintptr_t)va_arg(ap,void*);
            terminal_writestring("0x"); put_uint(v, 16, 0, 0, 0); break; }
        case 's': {
            const char *s = va_arg(ap,const char*); if (!s) s="(null)";
            int l=0; while(s[l]) l++;
            int pad=width-l; while(pad-->0) terminal_putchar(' ');
            terminal_writestring(s); break; }
        case 'c': terminal_putchar((char)va_arg(ap,int)); break;
        case '%': terminal_putchar('%'); break;
        default:  terminal_putchar(*(fmt-1)); break;
        }
    }
    va_end(ap);
    spin_unlock(&terminal_lock);
}




