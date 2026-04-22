


static void sysmon_on_draw(Window* win, void* app_data) {
    Rect r = win->rect;
    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, r.h - TITLE_H, 0xFF0A0A10);


    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, 24, 0xFF1A1A2A);
    fgl_draw_string(r.x + 10, r.y + TITLE_H + 5, "CPU", 0xFF00FF00, 0xFF1A1A2A);
    fgl_draw_string(r.x + 80, r.y + TITLE_H + 5, "RAM", 0xFFFFFFFF, 0xFF1A1A2A);
    fgl_draw_string(r.x + 150, r.y + TITLE_H + 5, "NET", 0xFFFFFFFF, 0xFF1A1A2A);
    fgl_draw_string(r.x + 220, r.y + TITLE_H + 5, "DISK", 0xFFFFFFFF, 0xFF1A1A2A);


    fgl_draw_rect_border(r.x + 10, r.y + TITLE_H + 34, r.w - 20, 100, 0xFF404060);
    fgl_draw_string(r.x + 20, r.y + TITLE_H + 40, "Load: [|||||     ] 45%", 0xFF00FF00, 0xFF0A0A10);
}

void module_main(void) {
    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = sysmon_on_draw;
    create_window((Rect){100, 100, 360, 240}, "System Monitor Pro", 0xFF0A0A10)->app = app;
}




