



static void ascii_on_draw(Window* win, void* app_data) {
    Rect r = win->rect;
    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, r.h - TITLE_H, 0xFF121218);
    for (int i = 32; i < 128; i++) {
        int col = (i - 32) % 8;
        int row = (i - 32) / 8;
        char buf[8]; buf[0] = (char)i; buf[1] = 0;
        fgl_draw_string(r.x + 10 + col * 40, r.y + TITLE_H + 10 + row * 18, buf, 0xFF00DDFF, 0xFF121218);
    }
}

void module_main(void) {
    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = ascii_on_draw;
    create_window((Rect){100, 100, 360, 280}, "ASCII Table", 0xFF121218)->app = app;
}




