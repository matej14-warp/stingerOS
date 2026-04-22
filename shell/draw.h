











typedef struct { uint8_t r,g,b,a; } rgba_t;




typedef struct {
    uint8_t pixels[CANVAS_H][CANVAS_W];
    int     cursor_x, cursor_y;
    int     color;
    int     modified;
    char    filename[64];
} canvas_t;

int   png_decode(const uint8_t *data, size_t len, rgba_t **out, uint32_t *width, uint32_t *height);
void  png_render_ansi(const rgba_t *pixels, uint32_t w, uint32_t h);
uint8_t rgba_to_vga16(rgba_t p);

int   canvas_save(canvas_t *c);
int   canvas_load(canvas_t *c, const char *filename);

void  cmd_draw(const char *filename);
void  cmd_view_png(const char *path);






