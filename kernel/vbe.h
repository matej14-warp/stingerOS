#ifndef VBE_H
#define VBE_H

/* scorpion OS - kernel/vbe.h
   VBE/VESA framebuffer via multiboot1 VBE fields.
   Request 1920x1080x32 in the multiboot header; GRUB fills
   vbe_mode_info at boot.  We read pitch/addr from there and
   provide a simple pixel + text terminal on top.             */

#include <stdint.h>
#include <stddef.h>

/* ---- VBE ModeInfoBlock (subset we care about) ---- */
typedef struct {
    uint16_t attributes;
    uint8_t  win_a, win_b;
    uint16_t granularity;
    uint16_t win_size;
    uint16_t seg_a, seg_b;
    uint32_t win_func_ptr;
    uint16_t pitch;          /* bytes per scanline — USE THIS, not width*bpp/8 */
    uint16_t width;          /* pixels per scanline                             */
    uint16_t height;
    uint8_t  w_char, y_char, planes;
    uint8_t  bpp;            /* bits per pixel                                  */
    uint8_t  banks;
    uint8_t  memory_model;
    uint8_t  bank_size, image_pages;
    uint8_t  reserved0;
    /* direct colour fields */
    uint8_t  red_mask,   red_pos;
    uint8_t  green_mask, green_pos;
    uint8_t  blue_mask,  blue_pos;
    uint8_t  rsv_mask,   rsv_pos;
    uint8_t  direct_colour_attrs;
    uint32_t framebuffer;    /* physical address of linear framebuffer          */
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

/* ---- Public API ---- */

/* Call early in kernel_main with the mbi vbe_mode_info_addr field.
   Returns 0 on success, -1 if VBE info missing/invalid.             */
int  vbe_init(uint32_t mode_info_phys);

/* 1 if framebuffer is active, 0 if we fell back to VGA text          */
int  vbe_active(void);

/* Read a pixel from the back buffer (RGB, no alpha)                  */
uint32_t vbe_get_pixel(int x, int y);

/* Put a single 32-bit ARGB pixel                                     */
void vbe_put_pixel(int x, int y, uint32_t colour);

/* Fill a rectangle                                                   */
void vbe_fill_rect(int x, int y, int w, int h, uint32_t colour);

/* Clear screen to colour                                             */
void vbe_clear(uint32_t colour);

/* Draw an 8x16 character from the built-in font                     */
void vbe_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);

/* Draw only foreground pixels (transparent background)               */
void vbe_draw_char_mask(int x, int y, char c, uint32_t fg);

/* Draw a NUL-terminated string; wraps and scrolls                   */
void vbe_draw_string(const char *s, uint32_t fg, uint32_t bg);

/* terminal_*-compatible wrappers so the rest of the kernel can use
   vbe_terminal_putchar / vbe_terminal_printf instead of swapping
   every call site at once.                                           */
void vbe_terminal_init(void);
void vbe_terminal_putchar(char c);
void vbe_terminal_writestring(const char *s);
void vbe_terminal_printf(const char *fmt, ...);
void vbe_terminal_clear(void);
void vbe_terminal_setcolor(uint8_t vga_color_byte);  /* accepts VGA attribute */
void vbe_terminal_update_cursor(void); /* blinks the cursor */

/* Blit back buffer to hardware framebuffer (call once per frame) */
void vbe_flip(void);
/* Partial flip — blit only [x,y,w,h] from bb to hardware fb.
   Use for cursor-only updates to avoid copying the full framebuffer. */
void vbe_flip_rect(int x, int y, int w, int h);

/* Control whether vbe_terminal_putchar auto-flips after each char.
   Call vbe_set_autoflip(0) before entering the GUI compositor so the
   terminal doesn't blit on every character and only vbe_flip() in the
   render loop does the hardware blit.  Default is 1 (auto-flip on).  */
void vbe_set_autoflip(int on);

/* Direct back-buffer pixel access for bulk blits (use with care)  */
void vbe_bb_blit_rect(int x, int y, int w, int h,
                       const uint32_t *pixels);  /* pixels in ARGB row-major */

/* Expose back-buffer geometry for fast direct writes in the GUI compositor */
static inline uint32_t *vbe_bb_row(int y) {
    extern uint32_t *g_bb;
    extern int       g_pitch;
    return g_bb + (unsigned)y * (unsigned)g_pitch;
}
extern int g_pitch;  /* pixels per scanline (exported for inline above) */

/* Screen geometry (valid after vbe_init succeeds)                 */
extern int vbe_screen_width;
extern int vbe_screen_height;

#endif /* VBE_H */