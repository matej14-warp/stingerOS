/* scorpion OS - kernel/vbe.c
   VBE/VESA linear framebuffer driver.

   Architecture:
   - Hardware framebuffer mapped at vbe_fb (physical addr from VBE mode info)
   - Software back-buffer (g_bb) allocated in BSS — same size as fb
   - All drawing goes to g_bb; vbe_flip() copies g_bb -> vbe_fb
   - vbe_flip_rect() does a partial copy for cursor-only updates
   - VBE terminal: 8x16 font, scrolling, ANSI escape sequences         */

#include "vbe.h"
#include "terminal.h"
#include "kmalloc.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* forward declarations */
static void vbe_scroll(int lines);

/* ---- screen geometry (exported) ---- */
int vbe_screen_width  = 0;
int vbe_screen_height = 0;

/* ---- internal state ---- */
static uint32_t *vbe_fb   = 0;   /* hardware framebuffer */
uint32_t        *g_bb     = 0;   /* back buffer (exported via vbe.h inline) */
int              g_pitch  = 0;   /* pixels per scanline  (exported via vbe.h) */
static int       g_active = 0;
static int       g_autoflip = 1;

/* ---- terminal state ---- */
static int  g_col = 0, g_row = 0;
static int  g_tcols = 0, g_trows = 0;
static uint32_t g_fg = 0x00AAAAAA;
static uint32_t g_bg = 0x00000000;

/* ANSI escape state */
static int g_esc_state = 0;  /* 0=normal 1=got-ESC 2=got-[ */
static int g_esc_params[8];
static int g_esc_param_count = 0;
static int g_esc_cur_valid = 0;
static int g_esc_cur_param = 0;

/* ---- 8x16 font (PC BIOS font subset, first 128 chars) ---- */
/* Each char is 16 bytes, 1 bit per pixel, MSB left */
static const uint8_t g_font[128][16] = {
    /* 0x00 */ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x01 */ {0,0,0x7E,0x81,0xA5,0x81,0x81,0xBD,0x99,0x81,0x81,0x7E,0,0,0,0},
    /* 0x02 */ {0,0,0x7E,0xFF,0xDB,0xFF,0xFF,0xC3,0xE7,0xFF,0xFF,0x7E,0,0,0,0},
    /* 0x03 */ {0,0,0,0x6C,0xFE,0xFE,0xFE,0xFE,0x7C,0x38,0x10,0,0,0,0,0},
    /* 0x04 */ {0,0,0,0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10,0,0,0,0,0,0},
    /* 0x05 */ {0,0,0x18,0x3C,0x3C,0xE7,0xE7,0xE7,0x18,0x18,0x3C,0,0,0,0,0},
    /* 0x06 */ {0,0,0x18,0x3C,0x7E,0xFF,0xFF,0x7E,0x18,0x18,0x3C,0,0,0,0,0},
    /* 0x07 */ {0,0,0,0,0x18,0x3C,0x3C,0x18,0,0,0,0,0,0,0,0},
    /* space - 0x20 filled below, entries 8-31 are control chars */
    [0x08]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x09]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x0A]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x0B]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x0C]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x0D]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x0E]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x0F]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x10]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x11]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x12]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x13]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x17]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x18]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x19]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x1A]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x1B]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x1C]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x1D]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x1E]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x1F]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    /* 0x20 - space */
    [' '] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    [0x21]={0,0,0x18,0x3C,0x3C,0x3C,0x18,0x18,0,0x18,0x18,0,0,0,0,0},   /* ! */
    [0x22]={0,0,0x66,0x66,0x66,0x24,0,0,0,0,0,0,0,0,0,0},               /* " */
    [0x23]={0,0,0x6C,0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x6C,0,0,0,0,0}, /* # */
    [0x24]={0x18,0x18,0x7C,0xC6,0xC2,0xC0,0x7C,0x06,0x06,0x86,0xC6,0x7C,0x18,0x18,0,0}, /* $ */
    [0x25]={0,0,0,0xC2,0xC6,0x0C,0x18,0x30,0x60,0xC6,0x86,0,0,0,0,0},   /* % */
    [0x26]={0,0,0x38,0x6C,0x6C,0x38,0x76,0xDC,0xCC,0xCC,0x76,0,0,0,0,0}, /* & */
    [0x27]={0,0,0x18,0x18,0x18,0x30,0,0,0,0,0,0,0,0,0,0},               /* ' */
    [0x28]={0,0,0x0C,0x18,0x30,0x30,0x30,0x30,0x30,0x18,0x0C,0,0,0,0,0}, /* ( */
    [0x29]={0,0,0x30,0x18,0x0C,0x0C,0x0C,0x0C,0x0C,0x18,0x30,0,0,0,0,0}, /* ) */
    [0x2A]={0,0,0,0,0,0x66,0x3C,0xFF,0x3C,0x66,0,0,0,0,0,0},            /* * */
    [0x2B]={0,0,0,0,0x18,0x18,0x7E,0x18,0x18,0,0,0,0,0,0,0},            /* + */
    [0x2C]={0,0,0,0,0,0,0,0,0,0x18,0x18,0x30,0,0,0,0},                  /* , */
    [0x2D]={0,0,0,0,0,0,0x7E,0,0,0,0,0,0,0,0,0},                        /* - */
    [0x2E]={0,0,0,0,0,0,0,0,0,0x18,0x18,0,0,0,0,0},                     /* . */
    [0x2F]={0,0,0,0,0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0,0,0,0},   /* / */
    /* 0x30-0x39 digits */
    [0x30]={0,0,0x3C,0x66,0xC3,0xC3,0xDB,0xC3,0xC3,0x66,0x3C,0,0,0,0,0}, /* 0 */
    [0x31]={0,0,0x18,0x38,0x58,0x18,0x18,0x18,0x18,0x18,0x7E,0,0,0,0,0}, /* 1 */
    [0x32]={0,0,0x7C,0xC6,0x06,0x0C,0x18,0x30,0x60,0xC6,0xFE,0,0,0,0,0}, /* 2 */
    [0x33]={0,0,0x7C,0xC6,0x06,0x06,0x3C,0x06,0x06,0xC6,0x7C,0,0,0,0,0}, /* 3 */
    [0x34]={0,0,0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x1E,0,0,0,0,0}, /* 4 */
    [0x35]={0,0,0xFE,0xC0,0xC0,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0,0,0,0,0}, /* 5 */
    [0x36]={0,0,0x38,0x60,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0}, /* 6 */
    [0x37]={0,0,0xFE,0xC6,0x06,0x06,0x0C,0x18,0x30,0x30,0x30,0,0,0,0,0}, /* 7 */
    [0x38]={0,0,0x7C,0xC6,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0}, /* 8 */
    [0x39]={0,0,0x7C,0xC6,0xC6,0xC6,0x7E,0x06,0x06,0x0C,0x78,0,0,0,0,0}, /* 9 */
    [0x3A]={0,0,0,0,0x18,0x18,0,0,0x18,0x18,0,0,0,0,0,0},               /* : */
    [0x3B]={0,0,0,0,0x18,0x18,0,0,0x18,0x18,0x30,0,0,0,0,0},            /* ; */
    [0x3C]={0,0,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0,0,0,0,0}, /* < */
    [0x3D]={0,0,0,0,0,0x7E,0,0x7E,0,0,0,0,0,0,0,0},                     /* = */
    [0x3E]={0,0,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0,0,0,0,0}, /* > */
    [0x3F]={0,0,0x7C,0xC6,0xC6,0x0C,0x18,0x18,0,0x18,0x18,0,0,0,0,0},   /* ? */
    [0x40]={0,0,0x7C,0xC6,0xC6,0xDE,0xDE,0xDE,0xDC,0xC0,0x7C,0,0,0,0,0}, /* @ */
    /* A-Z */
    [0x41]={0,0,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0,0,0,0,0}, /* A */
    [0x42]={0,0,0xFC,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0xFC,0,0,0,0,0}, /* B */
    [0x43]={0,0,0x3C,0x66,0xC2,0xC0,0xC0,0xC0,0xC2,0x66,0x3C,0,0,0,0,0}, /* C */
    [0x44]={0,0,0xF8,0x6C,0x66,0x66,0x66,0x66,0x66,0x6C,0xF8,0,0,0,0,0}, /* D */
    [0x45]={0,0,0xFE,0x62,0x60,0x64,0x7C,0x64,0x60,0x62,0xFE,0,0,0,0,0}, /* E */
    [0x46]={0,0,0xFE,0x66,0x62,0x64,0x7C,0x64,0x60,0x60,0xF0,0,0,0,0,0}, /* F */
    [0x47]={0,0,0x3C,0x66,0xC2,0xC0,0xDE,0xC6,0xC6,0x66,0x3A,0,0,0,0,0}, /* G */
    [0x48]={0,0,0xC6,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0,0,0,0,0}, /* H */
    [0x49]={0,0,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0}, /* I */
    [0x4A]={0,0,0x1E,0x0C,0x0C,0x0C,0x0C,0xCC,0xCC,0xCC,0x78,0,0,0,0,0}, /* J */
    [0x4B]={0,0,0xE6,0x66,0x6C,0x6C,0x78,0x6C,0x6C,0x66,0xE6,0,0,0,0,0}, /* K */
    [0x4C]={0,0,0xF0,0x60,0x60,0x60,0x60,0x62,0x66,0x66,0xFE,0,0,0,0,0}, /* L */
    [0x4D]={0,0,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0xC6,0xC6,0,0,0,0,0}, /* M */
    [0x4E]={0,0,0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0xC6,0xC6,0,0,0,0,0}, /* N */
    [0x4F]={0,0,0x38,0x6C,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0,0,0,0,0}, /* O */
    [0x50]={0,0,0xFC,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0xF0,0,0,0,0,0}, /* P */
    [0x51]={0,0,0x78,0xCC,0xCC,0xCC,0xCC,0xDC,0x78,0x1C,0x1E,0,0,0,0,0}, /* Q */
    [0x52]={0,0,0xFC,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0xE6,0,0,0,0,0}, /* R */
    [0x53]={0,0,0x7C,0xC6,0xC6,0x60,0x38,0x0C,0xC6,0xC6,0x7C,0,0,0,0,0}, /* S */
    [0x54]={0,0,0x7E,0x7E,0x5A,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0}, /* T */
    [0x55]={0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0}, /* U */
    [0x56]={0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0,0,0,0,0}, /* V */
    [0x57]={0,0,0xC6,0xC6,0xC6,0xD6,0xD6,0xFE,0xEE,0xC6,0xC6,0,0,0,0,0}, /* W */
    [0x58]={0,0,0xC6,0xC6,0x6C,0x6C,0x38,0x6C,0x6C,0xC6,0xC6,0,0,0,0,0}, /* X */
    [0x59]={0,0,0x66,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x3C,0,0,0,0,0}, /* Y */
    [0x5A]={0,0,0xFE,0xC6,0x86,0x0C,0x18,0x30,0x60,0xC2,0xFE,0,0,0,0,0}, /* Z */
    [0x5B]={0,0,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0,0,0,0,0}, /* [ */
    [0x5C]={0,0,0,0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0,0,0,0,0},   /* \ */
    [0x5D]={0,0,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0,0,0,0,0}, /* ] */
    [0x5E]={0x10,0x38,0x6C,0xC6,0,0,0,0,0,0,0,0,0,0,0,0},               /* ^ */
    [0x5F]={0,0,0,0,0,0,0,0,0,0,0,0xFF,0,0,0,0},                         /* _ */
    [0x60]={0,0x30,0x18,0x0C,0,0,0,0,0,0,0,0,0,0,0,0},                   /* ` */
    /* a-z */
    [0x61]={0,0,0,0,0,0x78,0x0C,0x7C,0xCC,0xCC,0x76,0,0,0,0,0},          /* a */
    [0x62]={0,0,0xE0,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0xDC,0,0,0,0,0}, /* b */
    [0x63]={0,0,0,0,0,0x7C,0xC6,0xC0,0xC0,0xC6,0x7C,0,0,0,0,0},          /* c */
    [0x64]={0,0,0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0xCC,0xCC,0x76,0,0,0,0,0}, /* d */
    [0x65]={0,0,0,0,0,0x7C,0xC6,0xFE,0xC0,0xC6,0x7C,0,0,0,0,0},          /* e */
    [0x66]={0,0,0x1C,0x36,0x30,0x30,0xFC,0x30,0x30,0x30,0x78,0,0,0,0,0}, /* f */
    [0x67]={0,0,0,0,0,0x76,0xCC,0xCC,0xCC,0x7C,0x0C,0xCC,0x78,0,0,0},    /* g */
    [0x68]={0,0,0xE0,0x60,0x60,0x6C,0x76,0x66,0x66,0x66,0xE6,0,0,0,0,0}, /* h */
    [0x69]={0,0,0x18,0x18,0,0x38,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0},    /* i */
    [0x6A]={0,0,0x06,0x06,0,0x0E,0x06,0x06,0x06,0x66,0x66,0x3C,0,0,0,0}, /* j */
    [0x6B]={0,0,0xE0,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0xE6,0,0,0,0,0}, /* k */
    [0x6C]={0,0,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,0,0,0}, /* l */
    [0x6D]={0,0,0,0,0,0xCC,0xFE,0xD6,0xD6,0xD6,0xC6,0,0,0,0,0},          /* m */
    [0x6E]={0,0,0,0,0,0xDC,0x66,0x66,0x66,0x66,0x66,0,0,0,0,0},          /* n */
    [0x6F]={0,0,0,0,0,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,0,0,0},          /* o */
    [0x70]={0,0,0,0,0,0xDC,0x66,0x66,0x66,0x7C,0x60,0x60,0xF0,0,0,0},    /* p */
    [0x71]={0,0,0,0,0,0x76,0xCC,0xCC,0xCC,0x7C,0x0C,0x0C,0x1E,0,0,0},    /* q */
    [0x72]={0,0,0,0,0,0xDC,0x76,0x66,0x60,0x60,0xF0,0,0,0,0,0},          /* r */
    [0x73]={0,0,0,0,0,0x7C,0xC6,0x70,0x1C,0xC6,0x7C,0,0,0,0,0},          /* s */
    [0x74]={0,0,0x10,0x30,0x30,0xFC,0x30,0x30,0x30,0x36,0x1C,0,0,0,0,0}, /* t */
    [0x75]={0,0,0,0,0,0xCC,0xCC,0xCC,0xCC,0xCC,0x76,0,0,0,0,0},          /* u */
    [0x76]={0,0,0,0,0,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0,0,0,0,0},          /* v */
    [0x77]={0,0,0,0,0,0xC6,0xC6,0xD6,0xFE,0xFE,0x6C,0,0,0,0,0},          /* w */
    [0x78]={0,0,0,0,0,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0,0,0,0,0},          /* x */
    [0x79]={0,0,0,0,0,0xC6,0xC6,0xC6,0xC6,0x7E,0x06,0x0C,0xF8,0,0,0},   /* y */
    [0x7A]={0,0,0,0,0,0xFE,0xCC,0x18,0x30,0x66,0xFE,0,0,0,0,0},          /* z */
    [0x7B]={0,0,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x0E,0,0,0,0,0}, /* { */
    [0x7C]={0,0,0x18,0x18,0x18,0x18,0,0x18,0x18,0x18,0x18,0,0,0,0,0},    /* | */
    [0x7D]={0,0,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x70,0,0,0,0,0}, /* } */
    [0x7E]={0,0,0x76,0xDC,0,0,0,0,0,0,0,0,0,0,0,0},                      /* ~ */
    [0x7F]={0,0,0,0,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0,0,0,0,0,0},          /* DEL */
};

/* ---- helpers ----
   High-throughput framebuffer copy/fill.

   The bottleneck when writing to video RAM over PCIe is the write-combining
   buffer: stores must fill a full 64-byte (16-dword) WC line before they
   flush.  Issuing a single large "rep movsl" over the whole scanline (or the
   whole frame) is the fastest approach in protected mode without SSE:
   - The hardware WC buffer coalesces consecutive 32-bit stores into 64-byte
     PCIe TLPs automatically.
   - Splitting into per-row calls (as the old vbe_flip_rect did) caused a
     WC-buffer flush between each short row, halving throughput.

   vbe_flip() now copies the entire backbuffer in one shot.
   vbe_flip_rect() copies contiguous scanline spans in one shot.              */

static inline void memcpy32(uint32_t *dst, const uint32_t *src, int count) {
    __asm__ volatile (
        "rep movsl"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
    );
}

static inline void memset32(uint32_t *dst, uint32_t val, int count) {
    __asm__ volatile (
        "rep stosl"
        : "+D"(dst), "+c"(count)
        : "a"(val)
        : "memory"
    );
}

/* ================================================================ INIT */

int vbe_init(uint32_t mode_info_phys) {
    if (!mode_info_phys) return -1;
    vbe_mode_info_t *mi = (vbe_mode_info_t *)mode_info_phys;
    if (!mi->framebuffer || !mi->width || !mi->height) return -1;

    vbe_screen_width  = mi->width;
    vbe_screen_height = mi->height;
    g_pitch           = mi->pitch / 4;  /* bytes->pixels (32bpp assumed) */
    vbe_fb            = (uint32_t *)(uintptr_t)mi->framebuffer;

    /* Allocate back buffer */
    g_bb = (uint32_t *)kmalloc((size_t)(vbe_screen_width * vbe_screen_height * 4));
    if (!g_bb) return -1;

    /* Clear both buffers */
    memset32(g_bb,    0, vbe_screen_width * vbe_screen_height);
    memset32(vbe_fb,  0, g_pitch * vbe_screen_height);

    g_active   = 1;
    g_autoflip = 1;
    g_tcols    = vbe_screen_width  / 8;
    g_trows    = vbe_screen_height / 16;
    return 0;
}

/* Set MTRR write-combining on a physical range.
   Finds a free variable MTRR and programs it for WC (type 1).
   Silently skips if MTRRs are not supported or all slots are full. */
static void mtrr_set_wc(uint32_t base, uint32_t size) {
    /* check MTRR support via CPUID */
    uint32_t edx;
    __asm__ volatile("cpuid" : "=d"(edx) : "a"(1) : "ebx","ecx");
    if (!(edx & (1<<12))) return; /* no MTRR support */

    /* round size up to next power of two (MTRR mask requirement) */
    uint32_t sz = 1;
    while (sz < size) sz <<= 1;

    /* read MTRR cap to find number of variable MTRRs */
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo),"=d"(hi) : "c"(0xFE));
    int vcnt = (int)(lo & 0xFF);
    if (vcnt > 8) vcnt = 8;

    /* find a free variable MTRR (valid bit = 0 in mask MSR) */
    for (int i = 0; i < vcnt; i++) {
        uint32_t mlo, mhi;
        __asm__ volatile("rdmsr" : "=a"(mlo),"=d"(mhi) : "c"(0x200 + i*2 + 1));
        if (mhi & 0x800) continue; /* already in use */
        /* write base MSR: base | type WC (1) */
        __asm__ volatile("wrmsr" :: "c"(0x200 + i*2),   "a"(base | 1), "d"(0));
        /* write mask MSR: ~(size-1) | valid bit 11 */
        uint32_t mask = (~(sz - 1)) | (1 << 11);
        __asm__ volatile("wrmsr" :: "c"(0x200 + i*2 + 1), "a"(mask),    "d"(0));
        return;
    }
}

/* Direct init from multiboot framebuffer fields (kernel.c uses this path) */
void vbe_init_direct(uint32_t fb_addr, uint32_t pitch, uint32_t width, uint32_t height) {
    vbe_screen_width  = (int)width;
    vbe_screen_height = (int)height;
    g_pitch           = (int)(pitch / 4);
    vbe_fb            = (uint32_t *)(uintptr_t)fb_addr;
    /* Enable write-combining on the framebuffer so VRAM writes are batched
       instead of going one 32-bit store at a time over the PCI bus.        */
    uint32_t fb_size = pitch * height;
    mtrr_set_wc(fb_addr, fb_size);
    g_bb = (uint32_t *)kmalloc(width * height * 4);
    if (!g_bb) return;
    memset32(g_bb,   0, (int)(width * height));
    memset32(vbe_fb, 0, (int)(pitch/4 * height));
    g_active   = 1;
    g_autoflip = 1;
    g_tcols    = (int)width  / 8;
    g_trows    = (int)height / 16;
}

int vbe_active(void) { return g_active; }
void vbe_set_autoflip(int on) { g_autoflip = on; }

/* ================================================================ DRAWING */

void vbe_put_pixel(int x, int y, uint32_t colour) {
    if (!g_active || !g_bb) return;
    if (x < 0 || y < 0 || x >= vbe_screen_width || y >= vbe_screen_height) return;
    g_bb[y * g_pitch + x] = colour;
}

uint32_t vbe_get_pixel(int x, int y) {
    if (!g_active || !g_bb) return 0;
    if (x < 0 || y < 0 || x >= vbe_screen_width || y >= vbe_screen_height) return 0;
    return g_bb[y * g_pitch + x];
}

void vbe_fill_rect(int x, int y, int w, int h, uint32_t colour) {
    if (!g_active || !g_bb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > vbe_screen_width)  w = vbe_screen_width  - x;
    if (y + h > vbe_screen_height) h = vbe_screen_height - y;
    if (w <= 0 || h <= 0) return;
    for (int row = 0; row < h; row++)
        memset32(&g_bb[(y + row) * g_pitch + x], colour, w);
}

void vbe_clear(uint32_t colour) {
    /* Fill the entire backbuffer in one shot */
    if (!g_active || !g_bb) return;
    memset32(g_bb, colour, g_pitch * vbe_screen_height);
}

void vbe_bb_blit_rect(int x, int y, int w, int h, const uint32_t *pixels) {
    if (!g_active || !g_bb) return;
    if (x < 0 || y < 0 || x + w > vbe_screen_width || y + h > vbe_screen_height) return;
    for (int row = 0; row < h; row++)
        memcpy32(&g_bb[(y + row) * g_pitch + x], pixels + row * w, w);
}

/* ================================================================ FONT / TEXT */

void vbe_draw_char(int x, int y, char ch, uint32_t fg, uint32_t bg) {
    if (!g_active || !g_bb) return;
    if (x < 0 || y < 0 || x + 8 > vbe_screen_width) goto slow;
    {
        uint8_t idx = (uint8_t)ch;
        if (idx >= 128) idx = '?';
        const uint8_t *glyph = g_font[idx];
        for (int row = 0; row < 16; row++) {
            int py = y + row;
            if (py >= vbe_screen_height) break;
            if (py < 0) continue;
            uint8_t bits = glyph[row];
            uint32_t *dst = &g_bb[py * g_pitch + x];
            dst[0] = (bits & 0x80) ? fg : bg;
            dst[1] = (bits & 0x40) ? fg : bg;
            dst[2] = (bits & 0x20) ? fg : bg;
            dst[3] = (bits & 0x10) ? fg : bg;
            dst[4] = (bits & 0x08) ? fg : bg;
            dst[5] = (bits & 0x04) ? fg : bg;
            dst[6] = (bits & 0x02) ? fg : bg;
            dst[7] = (bits & 0x01) ? fg : bg;
        }
        return;
    }
slow:;
    uint8_t idx = (uint8_t)ch;
    if (idx >= 128) idx = '?';
    const uint8_t *glyph = g_font[idx];
    for (int row = 0; row < 16; row++) {
        int py = y + row;
        if (py < 0 || py >= vbe_screen_height) continue;
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            int px = x + col;
            if (px < 0 || px >= vbe_screen_width) continue;
            g_bb[py * g_pitch + px] = (bits & (0x80 >> col)) ? fg : bg;
        }
    }
}

void vbe_draw_char_mask(int x, int y, char ch, uint32_t fg) {
    if (!g_active || !g_bb) return;
    if (x < 0 || y < 0 || x + 8 > vbe_screen_width || y + 16 > vbe_screen_height) return;
    uint8_t idx = (uint8_t)ch;
    if (idx >= 128) idx = '?';
    const uint8_t *glyph = g_font[idx];
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        if (!bits) continue;
        uint32_t *dst = &g_bb[(y + row) * g_pitch + x];
        if (bits & 0x80) dst[0] = fg;
        if (bits & 0x40) dst[1] = fg;
        if (bits & 0x20) dst[2] = fg;
        if (bits & 0x10) dst[3] = fg;
        if (bits & 0x08) dst[4] = fg;
        if (bits & 0x04) dst[5] = fg;
        if (bits & 0x02) dst[6] = fg;
        if (bits & 0x01) dst[7] = fg;
    }
}

void vbe_draw_string(const char *s, uint32_t fg, uint32_t bg) {
    int x = g_col * 8, y = g_row * 16;
    while (*s) {
        if (*s == '\n') {
            x = 0; g_col = 0;
            y += 16; g_row++;
        } else {
            vbe_draw_char(x, y, *s, fg, bg);
            x += 8; g_col++;
            if (g_col >= g_tcols) { g_col = 0; x = 0; y += 16; g_row++; }
        }
        if (g_row >= g_trows) { vbe_scroll(1); y -= 16; g_row--; }
        s++;
    }
}

/* ================================================================ FLIP
   Copy backbuffer -> framebuffer.
   vbe_flip()      : full-screen copy in one rep movsl (fastest possible)
   vbe_flip_rect() : copies a vertical band; if full-width, collapses to one
                     contiguous copy so the WC buffer fills max-size TLPs.  */

void vbe_flip(void) {
    if (!g_active || !g_bb || !vbe_fb) return;
    /* One single rep movsl over the entire frame -- WC coalesces into
       maximum-size PCIe TLPs automatically.                            */
    int total = g_pitch * vbe_screen_height;
    memcpy32(vbe_fb, g_bb, total);
}

void vbe_flip_rect(int x, int y, int w, int h) {
    if (!g_active || !g_bb || !vbe_fb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > vbe_screen_width)  w = vbe_screen_width  - x;
    if (y + h > vbe_screen_height) h = vbe_screen_height - y;
    if (w <= 0 || h <= 0) return;

    /* Full-width rect: one contiguous copy */
    if (w == vbe_screen_width && x == 0) {
        int offset = y * g_pitch;
        memcpy32(vbe_fb + offset, g_bb + offset, h * g_pitch);
        return;
    }

    /* Partial-width: copy each row in one rep movsl */
    for (int row = 0; row < h; row++) {
        int offset = (y + row) * g_pitch + x;
        memcpy32(vbe_fb + offset, g_bb + offset, w);
    }
}

/* ================================================================ VBE TERMINAL */

/* VGA color -> RGB */
static uint32_t vga_to_rgb(uint8_t c) {
    static const uint32_t pal[16] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
        0xAA0000, 0xAA00AA, 0x00AAAA, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
        0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
    };
    return pal[c & 0xF];
}

static void vbe_scroll(int lines) {
    if (!g_active || !g_bb) return;
    int line_pixels = g_pitch * 16;
    int move_lines  = g_trows - lines;
    if (move_lines > 0)
        memcpy32(g_bb, g_bb + lines * line_pixels, move_lines * line_pixels);
    /* clear vacated bottom lines */
    memset32(g_bb + move_lines * line_pixels, g_bg,
             lines * line_pixels);
}

void vbe_terminal_init(void) {
    g_col = 0; g_row = 0;
    g_fg  = 0x00AAAAAA;
    g_bg  = 0x00000000;
    g_esc_state = 0;
    if (g_active) vbe_clear(g_bg);
}

void vbe_terminal_clear(void) {
    if (!g_active) return;
    vbe_clear(g_bg);
    g_col = 0; g_row = 0;
}

void vbe_terminal_setcolor(uint8_t vga_attr) {
    g_fg = vga_to_rgb(vga_attr & 0xF);
    g_bg = vga_to_rgb((vga_attr >> 4) & 0xF);
}

void vbe_terminal_update_cursor(void) {
    /* blink cursor -- invert cell under cursor */
    static int blink = 0;
    blink ^= 1;
    int x = g_col * 8, y = g_row * 16 + 14;
    uint32_t c = blink ? g_fg : g_bg;
    for (int i = 0; i < 8; i++)
        vbe_put_pixel(x + i, y, c);
    if (g_autoflip) vbe_flip_rect(x, g_row * 16, 8, 16);
}

/* ANSI escape processing */
static void ansi_sgr(void) {
    for (int i = 0; i < g_esc_param_count; i++) {
        int p = g_esc_params[i];
        if (p == 0)  { g_fg = 0xAAAAAA; g_bg = 0x000000; }
        else if (p == 1)  { /* bold -- brighten fg */ g_fg |= 0x555555; }
        else if (p >= 30 && p <= 37) g_fg = vga_to_rgb((uint8_t)(p - 30));
        else if (p == 39) g_fg = 0xAAAAAA;
        else if (p >= 40 && p <= 47) g_bg = vga_to_rgb((uint8_t)(p - 40));
        else if (p == 49) g_bg = 0x000000;
        else if (p >= 90 && p <= 97) g_fg = vga_to_rgb((uint8_t)(p - 90 + 8));
        else if (p >= 100 && p <= 107) g_bg = vga_to_rgb((uint8_t)(p - 100 + 8));
    }
}

static void ansi_dispatch(char cmd) {
    switch (cmd) {
    case 'm': ansi_sgr(); break;
    case 'A': g_row -= (g_esc_params[0] ? g_esc_params[0] : 1); if (g_row < 0) g_row = 0; break;
    case 'B': g_row += (g_esc_params[0] ? g_esc_params[0] : 1); if (g_row >= g_trows) g_row = g_trows-1; break;
    case 'C': g_col += (g_esc_params[0] ? g_esc_params[0] : 1); if (g_col >= g_tcols) g_col = g_tcols-1; break;
    case 'D': g_col -= (g_esc_params[0] ? g_esc_params[0] : 1); if (g_col < 0) g_col = 0; break;
    case 'H': case 'f':
        g_row = (g_esc_params[0] ? g_esc_params[0]-1 : 0);
        g_col = (g_esc_param_count > 1 && g_esc_params[1] ? g_esc_params[1]-1 : 0);
        break;
    case 'J': vbe_terminal_clear(); break;
    case 'K': /* erase line from cursor */
        vbe_fill_rect(g_col * 8, g_row * 16, (g_tcols - g_col) * 8, 16, g_bg);
        if (g_autoflip) vbe_flip_rect(g_col * 8, g_row * 16, (g_tcols - g_col) * 8, 16);
        break;
    case 's': g_esc_cur_valid = 1; g_esc_cur_param = g_col | (g_row << 16); break;
    case 'u': if (g_esc_cur_valid) { g_col = g_esc_cur_param & 0xFFFF; g_row = (g_esc_cur_param >> 16) & 0xFFFF; } break;
    default: break;
    }
}

void vbe_terminal_putchar(char c) {
    if (!g_active) {
        terminal_putchar(c);
        return;
    }

    /* ANSI escape state machine */
    if (g_esc_state == 1) {
        if (c == '[') { g_esc_state = 2; g_esc_param_count = 0; g_esc_params[0] = 0; return; }
        g_esc_state = 0;
    }
    if (g_esc_state == 2) {
        if (c >= '0' && c <= '9') {
            g_esc_params[g_esc_param_count] = g_esc_params[g_esc_param_count] * 10 + (c - '0');
            return;
        } else if (c == ';') {
            if (g_esc_param_count < 7) { g_esc_param_count++; g_esc_params[g_esc_param_count] = 0; }
            return;
        } else {
            g_esc_param_count++;
            ansi_dispatch(c);
            g_esc_state = 0;
            return;
        }
    }
    if (c == '\x1b') { g_esc_state = 1; return; }

    if (c == '\n') {
        g_col = 0;
        g_row++;
    } else if (c == '\r') {
        g_col = 0;
    } else if (c == '\b') {
        if (g_col > 0) {
            g_col--;
            vbe_draw_char(g_col * 8, g_row * 16, ' ', g_fg, g_bg);
        }
    } else if (c == '\t') {
        g_col = (g_col + 8) & ~7;
        if (g_col >= g_tcols) { g_col = 0; g_row++; }
    } else {
        vbe_draw_char(g_col * 8, g_row * 16, c, g_fg, g_bg);
        if (g_autoflip)
            vbe_flip_rect(g_col * 8, g_row * 16, 8, 16);
        g_col++;
        if (g_col >= g_tcols) { g_col = 0; g_row++; }
        return; /* skip full flip below */
    }

    if (g_row >= g_trows) {
        vbe_scroll(1);
        g_row = g_trows - 1;
    }

    if (g_autoflip) vbe_flip();
}

void vbe_terminal_writestring(const char *s) {
    /* batch: draw all chars to backbuffer, flip entire dirty region once */
    if (!g_active) { while (*s) vbe_terminal_putchar(*s++); return; }
    int dirty_x1 = g_col * 8, dirty_y1 = g_row * 16;
    int dirty_x2 = dirty_x1,  dirty_y2 = dirty_y1 + 16;
    int had_dirty = 0;
    int old_autoflip = g_autoflip;
    g_autoflip = 0;  /* suppress per-char flips */
    while (*s) {
        int px = g_col * 8, py = g_row * 16;
        vbe_terminal_putchar(*s++);
        if (px < dirty_x1) dirty_x1 = px;
        if (py < dirty_y1) dirty_y1 = py;
        if (px + 8 > dirty_x2) dirty_x2 = px + 8;
        if (py + 16 > dirty_y2) dirty_y2 = py + 16;
        had_dirty = 1;
    }
    g_autoflip = old_autoflip;
    if (old_autoflip && had_dirty) {
        vbe_flip_rect(dirty_x1, dirty_y1,
                      dirty_x2 - dirty_x1,
                      dirty_y2 - dirty_y1 + 16);
    }
}

/* minimal printf for vbe terminal */
void vbe_terminal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    int i = 0;
    while (*fmt && i < 510) {
        if (*fmt != '%') { buf[i++] = *fmt++; continue; }
        fmt++;
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width*10 + (*fmt++ - '0');
        switch (*fmt++) {
        case 'd': {
            int v = va_arg(args, int);
            char tmp[12]; int ti = 0, neg = 0;
            if (v < 0) { neg = 1; v = -v; }
            if (v == 0) tmp[ti++] = '0';
            while (v) { tmp[ti++] = '0' + v%10; v /= 10; }
            if (neg) tmp[ti++] = '-';
            while (ti > 0) buf[i++] = tmp[--ti];
            break;
        }
        case 'u': {
            unsigned v = va_arg(args, unsigned);
            char tmp[12]; int ti = 0;
            if (v == 0) tmp[ti++] = '0';
            while (v) { tmp[ti++] = '0' + v%10; v /= 10; }
            while (ti > 0) buf[i++] = tmp[--ti];
            break;
        }
        case 'x': case 'X': {
            unsigned v = va_arg(args, unsigned);
            char tmp[12]; int ti = 0;
            const char *hex = (*(fmt-1) == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            if (v == 0) tmp[ti++] = '0';
            while (v) { tmp[ti++] = hex[v&0xF]; v >>= 4; }
            while (ti > 0) buf[i++] = tmp[--ti];
            break;
        }
        case 's': {
            const char *s = va_arg(args, const char*);
            if (!s) s = "(null)";
            while (*s && i < 510) buf[i++] = *s++;
            break;
        }
        case 'c': buf[i++] = (char)va_arg(args, int); break;
        case '%': buf[i++] = '%'; break;
        default: break;
        }
    }
    buf[i] = 0;
    va_end(args);
    vbe_terminal_writestring(buf);
}
