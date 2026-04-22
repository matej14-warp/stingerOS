





typedef struct {
    char code_buf[4096];
    int cur_line;
    int cur_col;
} IDEState;

static void ide_on_draw(Window* win, void* app_data) {
    Rect r = win->rect;

    fgl_fill_rect(r.x, r.y + TITLE_H, r.w, r.h - TITLE_H, 0xFF1E1E1E);


    fgl_fill_rect(r.x, r.y + r.h - 22, r.w, 22, 0xFF007ACC);
    fgl_draw_string(r.x + 10, r.y + r.h - 18, "Ready", 0xFFFFFFFF, 0xFF007ACC);


    fgl_fill_rect(r.x, r.y + TITLE_H, 150, r.h - TITLE_H - 22, 0xFF252526);
    fgl_draw_rect_border(r.x + 150, r.y + TITLE_H, 1, r.h - TITLE_H - 22, 0xFF3F3F46);
    fgl_draw_string(r.x + 10, r.y + TITLE_H + 10, "Solution Explorer", 0xFFCCCCCC, 0xFF252526);

    int lnw = 40;

    fgl_fill_rect(r.x + 150, r.y + TITLE_H, r.w - 150, r.h - TITLE_H - 22, 0xFF1E1E1E);


    fgl_fill_rect(r.x + 150, r.y + TITLE_H, lnw, r.h - TITLE_H - 22, 0xFF1E1E1E);
    fgl_draw_rect_border(r.x + 150 + lnw, r.y + TITLE_H, 1, r.h - TITLE_H - 22, 0xFF3F3F46);

    for (int i = 0; i < 15; i++) {
        char buf[8]; buf[0] = ' '; buf[1] = '0' + (i+1)/10; buf[2] = '0' + (i+1)%10; buf[3] = 0;
        fgl_draw_string(r.x + 150 + 4, r.y + TITLE_H + 10 + i * 16, buf, 0xFF2B91AF, 0xFF1E1E1E);
    }


    fgl_draw_string(r.x + 150 + lnw + 8, r.y + TITLE_H + 10, "using michealsoft;", 0xFF569CD6, 0xFF1E1E1E);
    fgl_draw_string(r.x + 150 + lnw + 8, r.y + TITLE_H + 26, "physical_studio_run();", 0xFFDCDCAA, 0xFF1E1E1E);
}

void module_main(void) {
    IDEState* state = (IDEState*)kmalloc(sizeof(IDEState));
    memset(state, 0, sizeof(IDEState));

    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = ide_on_draw;
    app->app_data = state;

    Window* win = create_window((Rect){120, 40, 700, 520}, "michealsoft physical studio", 0xFF1E1E1E);
    win->app = app;
}




