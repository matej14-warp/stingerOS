








typedef struct {
    uint16_t attributes;
    uint8_t  win_a, win_b;
    uint16_t granularity;
    uint16_t win_size;
    uint16_t seg_a, seg_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t  w_char, y_char, planes;
    uint8_t  bpp;
    uint8_t  banks;
    uint8_t  memory_model;
    uint8_t  bank_size, image_pages;
    uint8_t  reserved0;

    uint8_t  red_mask,   red_pos;
    uint8_t  green_mask, green_pos;
    uint8_t  blue_mask,  blue_pos;
    uint8_t  rsv_mask,   rsv_pos;
    uint8_t  direct_colour_attrs;
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;




int  vbe_init(uint32_t mode_info_phys);


int  vbe_active(void);


uint32_t vbe_get_pixel(int x, int y);


void vbe_put_pixel(int x, int y, uint32_t colour);


void vbe_fill_rect(int x, int y, int w, int h, uint32_t colour);


void vbe_clear(uint32_t colour);


void vbe_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);


void vbe_draw_char_mask(int x, int y, char c, uint32_t fg);


void vbe_draw_string(const char *s, uint32_t fg, uint32_t bg);


void vbe_terminal_init(void);
void vbe_terminal_putchar(char c);
void vbe_terminal_writestring(const char *s);
void vbe_terminal_printf(const char *fmt, ...);
void vbe_terminal_clear(void);
void vbe_terminal_setcolor(uint8_t vga_color_byte);
void vbe_terminal_update_cursor(void);


void vbe_flip(void);

void vbe_flip_rect(int x, int y, int w, int h);

void vbe_enable_hw_accel(void);


void vbe_set_autoflip(int on);


void vbe_bb_blit_rect(int x, int y, int w, int h, const uint32_t *pixels);


static inline uint32_t *vbe_bb_row(int y) {
    extern uint32_t *g_bb;
    extern int       g_pitch;
    return g_bb + (unsigned)y * (unsigned)g_pitch;
}
extern int g_pitch;


extern int vbe_screen_width;
extern int vbe_screen_height;





