


static void draw_on_draw(Window* win, void* app_data) {
    Rect r = win->rect;
    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, r.h - TITLE_H, 0xFFFFFFFF);
    fgl_draw_string(r.x + 10, r.y + TITLE_H + 10, "Canvas Ready", 0xFF000000, 0xFFFFFFFF);
}

void module_main(void) {
    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = draw_on_draw;
    create_window((Rect){100, 100, 400, 400}, "Neo-Paint", 0xFFFFFFFF)->app = app;
}




