



typedef struct {
    char path[256];
    int sel;
    int scroll;
    int count;

} FMState;

static void fm_on_draw(Window* win, void* app_data) {
    FMState* state = (FMState*)app_data;
    Rect r = win->rect;

    int sw = 140;
    fgl_fill_rect(r.x, r.y + TITLE_H, sw, r.h - TITLE_H, 0xFF181820);
    fgl_fill_rect(r.x + sw, r.y + TITLE_H, r.w - sw, r.h - TITLE_H, 0xFF0A0A0E);
    fgl_draw_rect_border(r.x + sw, r.y + TITLE_H, 1, r.h - TITLE_H, 0xFF303040);


    const char* tabs[] = {"System", "Storage", "Network", "Users"};
    for (int i = 0; i < 4; i++) {
        fgl_draw_string(r.x + 10, r.y + TITLE_H + 10 + i * 28, tabs[i], 0xFFBBBBCC, 0xFF181820);
    }


    fgl_draw_string(r.x + sw + 10, r.y + TITLE_H + 10, "Name", 0xFF606080, 0xFF0A0A0E);
    fgl_draw_rect_border(r.x + sw, r.y + TITLE_H + 28, r.w - sw, 1, 0xFF1A1A22);


    fgl_draw_string(r.x + sw + 10, r.y + TITLE_H + 40, ".. [UP]", 0xFFAAAAFF, 0xFF0A0A0E);
    fgl_draw_string(r.x + sw + 10, r.y + TITLE_H + 64, "bin/", 0xFFFFFFFF, 0xFF0A0A0E);
    fgl_draw_string(r.x + sw + 10, r.y + TITLE_H + 88, "kernel.elf", 0xFF00FF00, 0xFF0A0A0E);
}

void module_main(void) {
    FMState* state = (FMState*)kmalloc(sizeof(FMState));
    memset(state, 0, sizeof(FMState));
    strcpy(state->path, "/");

    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = fm_on_draw;
    app->on_event = NULL;
    app->app_data = state;

    Window* win = create_window((Rect){80, 80, 500, 360}, "stinger Explorer Pro", 0xFF0A0A0E);
    win->app = app;
}




