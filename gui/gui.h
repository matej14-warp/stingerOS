






void* memset(void* dest, int ch, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
char* strncpy(char* dest, const char* src, size_t n);
char* strcpy(char* dest, const char* src);


void fgl_fill_rect(int x, int y, int w, int h, uint32_t color);
void fgl_draw_string(int x, int y, const char* s, uint32_t color, uint32_t bg);
void fgl_draw_rect_border(int x, int y, int w, int h, uint32_t color);
int vbe_get_width(void);
int vbe_get_height(void);


struct Window;
struct Widget;
struct GUI_Context;
struct GUI_Event;



typedef struct {
    int x, y;
} Point;

typedef struct {
    int x, y, w, h;
} Rect;



typedef struct AppInterface {
    void (*on_draw)(struct Window* win, void* app_data);
    void (*on_event)(struct Window* win, struct GUI_Event* event, void* app_data);
    void (*on_destroy)(void* app_data);
    void* app_data;
} AppInterface;



typedef enum {
    EVENT_NONE,
    EVENT_MOUSE_CLICK,
    EVENT_KEY_PRESS,
    EVENT_RESIZE,
    EVENT_DRAG,
    EVENT_PAINT,
    EVENT_CLOSE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MENU_ITEM_CLICK
} GUI_EventType;

typedef struct GUI_Event {
    GUI_EventType type;
    Point         mouse_pos;
    int           mouse_button;
    char          key;
    Rect          new_rect;
    int           menu_item_id;
} GUI_Event;

typedef void (*EventHandler)(struct Window* win, GUI_Event* event);



typedef enum {
    WIDGET_TYPE_BUTTON,
    WIDGET_TYPE_LABEL,
    WIDGET_TYPE_TEXTBOX,
    WIDGET_TYPE_MENU,
    WIDGET_TYPE_MENU_ITEM,
    WIDGET_TYPE_CONTAINER
} WidgetType;

typedef struct Widget {
    WidgetType    type;
    Rect          rect;
    char*         text;
    uint32_t      fg_color;
    uint32_t      bg_color;
    struct Widget* next;
    void*         data;
    EventHandler  on_click;
    EventHandler  on_key;
    EventHandler  on_paint;
} Widget;

Widget* create_widget(WidgetType type, Rect rect, const char* text, uint32_t fg, uint32_t bg);
void    destroy_widget(Widget* widget);
void    draw_widget(struct Window* win, Widget* widget);












typedef enum {
    WINDOW_STATE_NORMAL,
    WINDOW_STATE_MINIMIZED,
    WINDOW_STATE_MAXIMIZED,
    WINDOW_STATE_CLOSED
} WindowState;


















































typedef struct{char name[FM_NAMELEN];int is_dir;}fm_entry_t;

typedef struct {
    uint32_t pixels[DRAW_H][DRAW_W];
    uint32_t fg_color;
    int tool;
    int drawing;
} draw_state_t;





typedef struct {
    int active;
    int mode;
    char path[256];
    char entries[FM_MAXENTRIES][FM_NAMELEN];
    int entry_count;
    int is_dir[FM_MAXENTRIES];
    int sel,scroll;
    char input[128];
    int input_len;
    char result_path[256];
    int result_confirmed;
} file_picker_t;

typedef struct Window {
    int alive;
    union {
        struct { int x,y,w,h; };
        Rect rect;
    };
    int drag,dox,doy;

    int resize_handle;
    int resize_ox,resize_oy;
    int resize_rx,resize_ry;
    int resize_rw,resize_rh;
    char title[MAX_WINDOW_TITLE];
    char buf[TBSZ];int bstart,blen;
    char inp[INPUT_MAX+2];int inlen;
    int scroll,app_type;
    char fm_path[256];
    fm_entry_t fm_entries[FM_MAXENTRIES];
    int fm_count,fm_sel,fm_scroll;
    char br_url[256];
    char br_buf[BR_BUFSZ];
    int br_len,br_scroll,br_loading;
    int settings_tab,settings_tab_extra;
    int net_tab;
    draw_state_t *draw;
    volatile int busy;
    char queued_cmd[INPUT_MAX+2];
    char esc[4]; int esclen;
    int nano_cur_row,nano_cur_col;
    file_picker_t *file_picker;
    char loaded_file[256];
    uint32_t *image_data;
    int image_w, image_h;
    int fm_tab;
    AppInterface* app_iface;


    uint32_t      background_color;
    WindowState   state;
    int           is_draggable;
    int           is_resizable;
    int           is_focused;
    void          (*event_handler)(struct Window* win, struct GUI_Event* event, void* app_data);
    Widget*       widgets[MAX_WIDGETS_PER_WINDOW];
    int           widget_count;
    int           is_dragging;
    Point         drag_offset;
    int           is_resizing;
    Point         resize_start_pos;
    Rect          resize_start_rect;
    uint32_t      border_color_active;
    uint32_t      border_color_inactive;
    uint32_t      title_bar_color_active;
    uint32_t      title_bar_color_inactive;
    void*         rendering_context;
    int           focused_window_idx;


    int           dirty;
    Rect          last_rect;


    union {
        AppInterface* app;
    };
} Window;

Window* create_window(Rect rect, const char* title, uint32_t bg_color);
void    destroy_window(Window* win);
void    add_widget_to_window(Window* win, Widget* widget);
void    set_window_focus(Window* win);
void    draw_window(Window* win);
void    handle_window_event(Window* win, GUI_Event* event);
void    resize_window(Window* win, Rect new_rect);



typedef struct GUI_Context {
    Window* windows[16];
    int     window_count;
    int     focused_window_idx;
    Point   mouse_pos;
    uint8_t mouse_buttons;
} GUI_Context;

GUI_Context* gui_init(void);
void         gui_shutdown(GUI_Context* ctx);
Window*      gui_create_window(GUI_Context* ctx, Rect rect, const char* title, uint32_t bg_color);
void         gui_process_event(GUI_Context* ctx, GUI_Event* event);
void         gui_draw(GUI_Context* ctx);

int is_point_in_rect(Point p, Rect r);






