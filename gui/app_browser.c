



static void browser_on_draw(Window* win, void* app_data) {
    Rect r = win->rect;

    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, 32, 0xFF282832);
    fgl_draw_string(r.x + 10, r.y + TITLE_H + 8, "http://yaoi.net", 0xFFFFFFFF, 0xFF282832);


    fgl_fill_rect(r.x, r.y + TITLE_H + 32, r.w, r.h - TITLE_H - 32, 0xFFF0F0F5);
    fgl_draw_string(r.x + 20, r.y + TITLE_H + 50, "Welcome to the Web", 0xFF101010, 0xFFF0F0F5);
}

void module_main(void) {
    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = browser_on_draw;
    create_window((Rect){40, 40, 640, 480}, "stinger Navigator", 0xFFFFFFFF)->app = app;
}




