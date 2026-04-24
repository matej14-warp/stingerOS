/* scorpion OS - kernel/terminal.c
   VGA text-mode terminal.  When VBE is active, all output is
   forwarded to vbe_terminal_putchar() instead.
   Provides terminal_printf() with a full %d/%u/%x/%s/%c format set. */

#include "terminal.h"
#include "vbe.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
static volatile uint16_t * const VGA_BUF = (volatile uint16_t *)0xB8000;

static size_t  t_col   = 0;
static size_t  t_row   = 0;
static uint8_t t_color = 0x07;   /* light grey on black */

static terminal_hook_fn t_hook = NULL;
void terminal_set_hook(terminal_hook_fn fn) { t_hook = fn; }

static void (*t_clear_hook)(void) = NULL;
void terminal_set_clear_hook(void (*fn)(void)) { t_clear_hook = fn; }

static inline uint16_t vga_entry(char c, uint8_t col) {
    return (uint16_t)(unsigned char)c | ((uint16_t)col << 8);
}

static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_BUF[(y-1)*VGA_WIDTH+x] = VGA_BUF[y*VGA_WIDTH+x];
    for (size_t x = 0; x < VGA_WIDTH; x++)
        VGA_BUF[(VGA_HEIGHT-1)*VGA_WIDTH+x] = vga_entry(' ', t_color);
}

void terminal_init(void) {
    t_col = 0; t_row = 0;
    t_color = terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (size_t y=0;y<VGA_HEIGHT;y++)
        for (size_t x=0;x<VGA_WIDTH;x++)
            VGA_BUF[y*VGA_WIDTH+x] = vga_entry(' ', t_color);
}

void terminal_clear(void) {
    if (t_clear_hook) { t_clear_hook(); return; }
    if (vbe_active()) { vbe_terminal_clear(); return; }
    for (size_t y=0;y<VGA_HEIGHT;y++)
        for (size_t x=0;x<VGA_WIDTH;x++)
            VGA_BUF[y*VGA_WIDTH+x] = vga_entry(' ', t_color);
    t_col = 0; t_row = 0;
}

void terminal_setcolor(uint8_t color) {
    t_color = color;
    if (vbe_active()) vbe_terminal_setcolor(color);
}

void terminal_putchar(char c) {
    if (t_hook) t_hook(c);
    if (vbe_active()) { vbe_terminal_putchar(c); return; }
    if (c == '\n') {
        t_col = 0;
        if (++t_row == VGA_HEIGHT) { vga_scroll(); t_row = VGA_HEIGHT-1; }
        return;
    }
    if (c == '\r') { t_col = 0; return; }
    if (c == '\b') {
        if (t_col > 0) { t_col--; VGA_BUF[t_row*VGA_WIDTH+t_col] = vga_entry(' ', t_color); }
        return;
    }
    if (c == '\t') {
        t_col = (t_col + 8) & ~(size_t)7;
        if (t_col >= VGA_WIDTH) { t_col = 0; if (++t_row == VGA_HEIGHT) { vga_scroll(); t_row=VGA_HEIGHT-1; } }
        return;
    }
    VGA_BUF[t_row*VGA_WIDTH+t_col] = vga_entry(c, t_color);
    if (++t_col == VGA_WIDTH) {
        t_col = 0;
        if (++t_row == VGA_HEIGHT) { vga_scroll(); t_row = VGA_HEIGHT-1; }
    }
}

void terminal_write(const char *data, size_t size) {
    for (size_t i=0;i<size;i++) terminal_putchar(data[i]);
}

void terminal_writestring(const char *data) {
    while (*data) terminal_putchar(*data++);
}

/* ---- printf ---- */
static void put_uint(unsigned long long v, int base, int upper, int width, int zero_pad) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char buf[32]; int i = 0;
    if (!v) buf[i++] = '0';
    while (v) { buf[i++] = digits[v % (unsigned)base]; v /= (unsigned)base; }
    int pad = width - i;
    while (pad-- > 0) terminal_putchar(zero_pad ? '0' : ' ');
    while (i > 0) terminal_putchar(buf[--i]);
}

void terminal_printf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    while (*fmt) {
        if (*fmt != '%') { terminal_putchar(*fmt++); continue; }
        fmt++;
        int zero_pad = 0, width = 0;
        if (*fmt == '0') { zero_pad = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width*10 + (*fmt++ - '0'); }
        /* Long modifier */
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
}
