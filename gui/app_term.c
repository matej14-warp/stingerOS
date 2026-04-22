



typedef struct {
    char terminal_output[2048];
    int cursor_pos;
} TermState;

static void term_on_draw(Window* win, void* app_data) {
    TermState* state = (TermState*)app_data;
    Rect r = win->rect;


    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, r.h - TITLE_H, 0xFF0C0C0C);


    fgl_draw_string(r.x + 12, r.y + TITLE_H + 12, "stinger Shell [Version 6.0.442]", 0xFFE5E5E5, 0xFF0C0C0C);
    fgl_draw_string(r.x + 12, r.y + TITLE_H + 32, "C:\\Users\\boykisser>", 0xFFCCCCCC, 0xFF0C0C0C);


    uint32_t now = get_ticks();
    if ((now / 500) % 2 == 0) {
        fgl_fill_rect(r.x + 160, r.y + TITLE_H + 32, 8, 16, C_ACCENT);
    }
}

void module_main(void) {
    TermState* state = (TermState*)kmalloc(sizeof(TermState));
    memset(state, 0, sizeof(TermState));

    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = term_on_draw;
    app->on_event = NULL;
    app->on_destroy = NULL;
    app->app_data = state;

    Rect r = {50, 50, 400, 300};
    Window* win = create_window(r, "Modular Terminal", 0xFF080808);
    win->app = app;
}




