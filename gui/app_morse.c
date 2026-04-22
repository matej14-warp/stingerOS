



static void morse_on_draw(Window* win, void* app_data) {
    Rect r = win->rect;
    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, r.h - TITLE_H, 0xFF080814);
    fgl_draw_string(r.x + 10, r.y + TITLE_H + 10, "STATION: SCO-INTEL", 0xFF00FFCC, 0xFF080814);
    fgl_draw_string(r.x + 10, r.y + TITLE_H + 40, "... --- ...", 0xFFFFFFFF, 0xFF080814);
}

void module_main(void) {
    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = morse_on_draw;
    create_window((Rect){50, 50, 300, 200}, "Signal Decoder", 0xFF080814)->app = app;
}




