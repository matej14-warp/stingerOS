







typedef int wapi_handle_t;










typedef struct {
    int type;
    char key;
    int  mx, my;
    int  btn;
    int  w, h;
} wapi_event_t;




void wapi_init(void);


wapi_handle_t wapi_create(const char *title, int x, int y, int w, int h);


void wapi_destroy(wapi_handle_t h);


int wapi_alive(wapi_handle_t h);


void wapi_clear(wapi_handle_t h, uint32_t colour);
void wapi_fill_rect(wapi_handle_t h, int x, int y, int w, int ht, uint32_t colour);
void wapi_draw_text(wapi_handle_t h, int x, int y, const char *s, uint32_t fg);
void wapi_draw_char(wapi_handle_t h, int x, int y, char c, uint32_t fg);
void wapi_put_pixel(wapi_handle_t h, int x, int y, uint32_t colour);


void wapi_flush(wapi_handle_t h);


int wapi_width(wapi_handle_t h);
int wapi_height(wapi_handle_t h);


int wapi_poll(wapi_handle_t h, wapi_event_t *ev);


void wapi_push_key(wapi_handle_t h, char k);
void wapi_push_click(wapi_handle_t h, int mx, int my, int btn);
void wapi_push_close(wapi_handle_t h);


void wapi_redraw_all(void);


void wapi_handle_click(int mx, int my, int btn);


void wapi_handle_key(char k);






