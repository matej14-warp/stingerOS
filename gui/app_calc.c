




typedef struct {
    char input[64];
    int inlen;
    double result;
} CalcState;

static void calc_on_draw(Window* win, void* app_data) {
    CalcState* state = (CalcState*)app_data;
    Rect r = win->rect;


    Rect disp = {r.x + 10, r.y + TITLE_H + 10, r.w - 20, 40};
    fgl_fill_rect(disp.x, disp.y, disp.w, disp.h, 0xFF202020);
    fgl_draw_string(disp.x + 5, disp.y + 12, state->input, 0xFFFFFFFF, 0xFF202020);


    const char* labels[] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "0", ".", "C", "+"
    };

    int bw = (r.w - 30) / 4;
    int bh = 30;
    int start_y = disp.y + disp.h + 10;

    for (int i = 0; i < 16; i++) {
        int row = i / 4;
        int col = i % 4;
        Rect btn_r = {r.x + 10 + col * (bw + 3), start_y + row * (bh + 3), bw, bh};
        fgl_fill_rect(btn_r.x, btn_r.y, btn_r.w, btn_r.h, 0xFF404040);
        fgl_draw_string(btn_r.x + (bw - 8) / 2, btn_r.y + (bh - 16) / 2, labels[i], 0xFFFFFFFF, 0xFF404040);
    }


    Rect eq_r = {r.x + 10, start_y + 4 * (bh + 3), r.w - 20, 30};
    fgl_fill_rect(eq_r.x, eq_r.y, eq_r.w, eq_r.h, 0xFFA050FF);
    fgl_draw_string(eq_r.x + (r.w - 20 - 48) / 2, eq_r.y + 7, "EQUALS", 0xFFFFFFFF, 0xFFA050FF);
}

static void calc_on_event(Window* win, GUI_Event* event, void* app_data) {
    CalcState* state = (CalcState*)app_data;
    if (event->type == EVENT_MOUSE_CLICK) {


    }
}

void module_main(void) {
    CalcState* state = (CalcState*)kmalloc(sizeof(CalcState));
    memset(state, 0, sizeof(CalcState));
    strcpy(state->input, "0");

    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = calc_on_draw;
    app->on_event = calc_on_event;
    app->on_destroy = NULL;
    app->app_data = state;

    Rect r = {100, 100, 240, 320};
    Window* win = create_window(r, "stinger Calc v4", 0xFF181020);
    win->app = app;


}




