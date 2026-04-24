/* scorpion OS - kernel/terminal.c
   VGA text-mode driver (80x25) with ANSI/VT100 escape sequence support.

   Supported sequences:
     ESC[A/B/C/D        cursor up/down/right/left
     ESC[H  ESC[f       cursor home
     ESC[<r>;<c>H       cursor position
     ESC[J  ESC[2J      clear screen
     ESC[K              erase to end of line
     ESC[<n>m           SGR: 0=reset 1=bold 30-37=fg 40-47=bg
     ESC[s / ESC[u      save/restore cursor                        */

#include "terminal.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEM    ((volatile uint16_t*)0xB8000)

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* ---- State ---- */
static size_t   terminal_row;
static size_t   terminal_col;
static uint8_t  terminal_color;
static uint8_t  terminal_default_color;
static volatile uint16_t *terminal_buffer;

/* Saved cursor position (ESC[s / ESC[u) */
static size_t   saved_row;
static size_t   saved_col;

/* ANSI escape parser state */
#define ESC_STATE_NORMAL  0
#define ESC_STATE_ESC     1   /* received 0x1B */
#define ESC_STATE_CSI     2   /* received 0x1B 0x5B */
#define ESC_MAX_PARAMS    8

static int     esc_state;
static int     esc_params[ESC_MAX_PARAMS];
static int     esc_param_count;
static int     esc_cur_param;   /* building current number */
static int     esc_cur_valid;   /* whether we've seen any digit */

/* ---- ANSI colour mapping (30-37 fg, 40-47 bg) ---- */
static vga_color_t ansi_to_vga[8] = {
    VGA_COLOR_BLACK, VGA_COLOR_RED,     VGA_COLOR_GREEN, VGA_COLOR_BROWN,
    VGA_COLOR_BLUE,  VGA_COLOR_MAGENTA, VGA_COLOR_CYAN,  VGA_COLOR_LIGHT_GREY
};

uint8_t terminal_make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)(unsigned char)c | ((uint16_t)color << 8);
}

void terminal_set_cursor(int x, int y) {
    uint16_t pos = (uint16_t)(y * VGA_WIDTH + x);
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void terminal_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            terminal_buffer[(y-1)*VGA_WIDTH + x] = terminal_buffer[y*VGA_WIDTH + x];
    for (size_t x = 0; x < VGA_WIDTH; x++)
        terminal_buffer[(VGA_HEIGHT-1)*VGA_WIDTH + x] = make_entry(' ', terminal_color);
    terminal_row = VGA_HEIGHT - 1;
}

void terminal_init(void) {
    terminal_row    = 0;
    terminal_col    = 0;
    terminal_color  = terminal_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_default_color = terminal_color;
    terminal_buffer = VGA_MEM;
    esc_state       = ESC_STATE_NORMAL;
    saved_row = saved_col = 0;
    terminal_clear();
}

void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            terminal_buffer[y * VGA_WIDTH + x] = make_entry(' ', terminal_color);
    terminal_row = 0; terminal_col = 0;
    terminal_set_cursor(0, 0);
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

/* ---- SGR handler ---- */
static void handle_sgr(void) {
    vga_color_t fg = (vga_color_t)(terminal_color & 0x0F);
    vga_color_t bg = (vga_color_t)((terminal_color >> 4) & 0x0F);

    int count = esc_param_count > 0 ? esc_param_count : 1;
    for (int i = 0; i < count; i++) {
        int p = esc_params[i];
        if (p == 0) {
            /* reset */
            terminal_color = terminal_default_color;
            return;
        } else if (p == 1) {
            /* bold → use bright version of current fg */
            fg = (vga_color_t)(fg | 8);
        } else if (p >= 30 && p <= 37) {
            fg = ansi_to_vga[p - 30];
        } else if (p == 39) {
            fg = (vga_color_t)(terminal_default_color & 0x0F);
        } else if (p >= 40 && p <= 47) {
            bg = ansi_to_vga[p - 40];
        } else if (p == 49) {
            bg = (vga_color_t)((terminal_default_color >> 4) & 0x0F);
        } else if (p >= 90 && p <= 97) {
            /* bright fg */
            fg = (vga_color_t)(ansi_to_vga[p - 90] | 8);
        } else if (p >= 100 && p <= 107) {
            /* bright bg */
            bg = (vga_color_t)(ansi_to_vga[p - 100] | 8);
        }
    }
    terminal_color = terminal_make_color(fg, bg);
}

/* ---- CSI final byte dispatcher ---- */
static void handle_csi(char cmd) {
    int p0 = esc_params[0];
    int p1 = esc_params[1];

    switch (cmd) {
    case 'A': /* cursor up */
        { int n = p0 ? p0 : 1;
          terminal_row = (size_t)n > terminal_row ? 0 : terminal_row - (size_t)n; }
        break;
    case 'B': /* cursor down */
        { int n = p0 ? p0 : 1;
          terminal_row += (size_t)n;
          if (terminal_row >= VGA_HEIGHT) terminal_row = VGA_HEIGHT - 1; }
        break;
    case 'C': /* cursor right */
        { int n = p0 ? p0 : 1;
          terminal_col += (size_t)n;
          if (terminal_col >= VGA_WIDTH) terminal_col = VGA_WIDTH - 1; }
        break;
    case 'D': /* cursor left */
        { int n = p0 ? p0 : 1;
          terminal_col = (size_t)n > terminal_col ? 0 : terminal_col - (size_t)n; }
        break;
    case 'H': case 'f': /* cursor position ESC[row;colH (1-based) */
        terminal_row = p0 > 0 ? (size_t)(p0 - 1) : 0;
        terminal_col = p1 > 0 ? (size_t)(p1 - 1) : 0;
        if (terminal_row >= VGA_HEIGHT) terminal_row = VGA_HEIGHT - 1;
        if (terminal_col >= VGA_WIDTH)  terminal_col = VGA_WIDTH - 1;
        break;
    case 'J': /* erase display */
        if (p0 == 2 || p0 == 0) terminal_clear();
        break;
    case 'K': /* erase line (to end) */
        for (size_t x = terminal_col; x < VGA_WIDTH; x++)
            terminal_buffer[terminal_row * VGA_WIDTH + x] = make_entry(' ', terminal_color);
        break;
    case 'm': /* SGR */
        handle_sgr();
        break;
    case 's': /* save cursor */
        saved_row = terminal_row; saved_col = terminal_col;
        break;
    case 'u': /* restore cursor */
        terminal_row = saved_row; terminal_col = saved_col;
        break;
    default:
        break;
    }
}

/* ---- Core putchar with escape parser ---- */
void terminal_putchar(char c) {
    uint8_t ch = (uint8_t)c;

    /* ---- ESC sequence state machine ---- */
    if (esc_state == ESC_STATE_ESC) {
        if (ch == '[') {
            esc_state = ESC_STATE_CSI;
            for (int i = 0; i < ESC_MAX_PARAMS; i++) esc_params[i] = 0;
            esc_param_count = 0;
            esc_cur_param   = 0;
            esc_cur_valid   = 0;
            return;
        } else if (ch == '7') { saved_row=terminal_row; saved_col=terminal_col; esc_state=ESC_STATE_NORMAL; return; }
        else if (ch == '8') { terminal_row=saved_row; terminal_col=saved_col; esc_state=ESC_STATE_NORMAL; return; }
        else { esc_state = ESC_STATE_NORMAL; }
    }

    if (esc_state == ESC_STATE_CSI) {
        if (ch >= '0' && ch <= '9') {
            esc_cur_param = esc_cur_param * 10 + (ch - '0');
            esc_cur_valid = 1;
            return;
        } else if (ch == ';') {
            if (esc_param_count < ESC_MAX_PARAMS) {
                esc_params[esc_param_count++] = esc_cur_param;
            }
            esc_cur_param = 0; esc_cur_valid = 0;
            return;
        } else {
            /* Final byte */
            if (esc_cur_valid && esc_param_count < ESC_MAX_PARAMS)
                esc_params[esc_param_count++] = esc_cur_param;
            esc_state = ESC_STATE_NORMAL;
            handle_csi((char)ch);
            terminal_set_cursor((int)terminal_col, (int)terminal_row);
            return;
        }
    }

    if (ch == 0x1B) { esc_state = ESC_STATE_ESC; return; }

    /* ---- Normal character ---- */
    switch (c) {
    case '\n':
        terminal_col = 0;
        if (++terminal_row == VGA_HEIGHT) terminal_scroll();
        break;
    case '\r':
        terminal_col = 0;
        break;
    case '\t':
        terminal_col = (terminal_col + 8) & ~(size_t)7;
        if (terminal_col >= VGA_WIDTH) {
            terminal_col = 0;
            if (++terminal_row == VGA_HEIGHT) terminal_scroll();
        }
        break;
    case '\b':
        if (terminal_col > 0) {
            terminal_col--;
            terminal_buffer[terminal_row * VGA_WIDTH + terminal_col] =
                make_entry(' ', terminal_color);
        }
        break;
    default:
        if (ch >= 32) {
            terminal_buffer[terminal_row * VGA_WIDTH + terminal_col] =
                make_entry(c, terminal_color);
            if (++terminal_col == VGA_WIDTH) {
                terminal_col = 0;
                if (++terminal_row == VGA_HEIGHT) terminal_scroll();
            }
        }
        break;
    }
    terminal_set_cursor((int)terminal_col, (int)terminal_row);
}

void terminal_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) terminal_putchar(data[i]);
}

void terminal_writestring(const char *data) {
    while (*data) terminal_putchar(*data++);
}

/* ---- Minimal printf: %s %d %u %x %c %% ---- */
void terminal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while (*fmt) {
        if (*fmt != '%') { terminal_putchar(*fmt++); continue; }
        fmt++;
        switch (*fmt++) {
        case 's': {
            const char *s = va_arg(args, const char*);
            if (!s) s = "(null)";
            terminal_writestring(s);
            break;
        }
        case 'd': {
            int v = va_arg(args, int);
            if (v < 0) { terminal_putchar('-'); v = -v; }
            char buf[12]; int i = 0;
            if (v == 0) { terminal_putchar('0'); break; }
            while (v) { buf[i++] = (char)('0' + v % 10); v /= 10; }
            while (i--) terminal_putchar(buf[i]);
            break;
        }
        case 'u': {
            unsigned v = va_arg(args, unsigned);
            char buf[12]; int i = 0;
            if (v == 0) { terminal_putchar('0'); break; }
            while (v) { buf[i++] = (char)('0' + v % 10); v /= 10; }
            while (i--) terminal_putchar(buf[i]);
            break;
        }
        case 'x': {
            unsigned v = va_arg(args, unsigned);
            char buf[9]; int i = 0;
            if (v == 0) { terminal_writestring("0"); break; }
            while (v) { int d = (int)(v & 0xF); buf[i++] = (char)(d < 10 ? '0'+d : 'a'+(d-10)); v >>= 4; }
            while (i--) terminal_putchar(buf[i]);
            break;
        }
        case 'c': terminal_putchar((char)va_arg(args, int)); break;
        case '%': terminal_putchar('%'); break;
        default:  terminal_putchar('?'); break;
        }
    }
    va_end(args);
}
