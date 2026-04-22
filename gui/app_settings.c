




typedef enum {
    TAB_NETWORK,
    TAB_WIFI,
    TAB_SYSTEM,
    TAB_DISPLAY,
    TAB_THEME,
    TAB_COUNT
} SettingsTab;

typedef struct {
    SettingsTab active_tab;
    int scanning;
    int selected_ap;
    char connect_status[64];
} SettingsState;

static const char* tab_names[] = {"Network", "WiFi", "System", "Display", "Theme"};

static void draw_wifi_tab(Window* win, SettingsState* state, Rect context_rect) {
    int x = context_rect.x + 20;
    int y = context_rect.y + 20;

    fgl_draw_string(x, y, "WiFi Utility", 0xFFFFFFFF, 0xFF08080C);


    Rect scan_btn = {x, y + 30, 80, 24};
    fgl_fill_rect(scan_btn.x, scan_btn.y, scan_btn.w, scan_btn.h, state->scanning ? 0xFF303040 : 0xFF404050);
    fgl_draw_rect_border(scan_btn.x, scan_btn.y, scan_btn.w, scan_btn.h, 0xFF606070);
    fgl_draw_string(scan_btn.x + 12, scan_btn.y + 4, state->scanning ? "..." : "Scan", 0xFFFFFFFF, 0x00000000);


    fgl_draw_string(x + 100, y + 34, state->connect_status, 0xFF88AA88, 0xFF08080C);


    y += 70;
    fgl_draw_string(x, y, "Available Networks:", 0xFF8888AA, 0xFF08080C);
    y += 24;

    if (wifi_ap_count == 0) {
        fgl_draw_string(x + 10, y, "No networks found. Click Scan.", 0xFF555566, 0xFF08080C);
    } else {
        for (int i = 0; i < wifi_ap_count && i < 8; i++) {
            uint32_t bg = (state->selected_ap == i) ? 0xFF202030 : 0xFF08080C;
            fgl_fill_rect(x, y + i * 24, win->rect.w - 200, 22, bg);

            char line[64];
            int rssi = wifi_aps[i].rssi;
            const char* sec = wifi_aps[i].encrypted ? "[

            fgl_draw_string(x + 5, y + i * 24 + 3, wifi_aps[i].ssid, 0xFFEEEEEE, bg);


            char sig[16] = "Signal: ";
            if (rssi > -50) strcpy(sig+8, "Excellent");
            else if (rssi > -70) strcpy(sig+8, "Good");
            else strcpy(sig+8, "Poor");

            fgl_draw_string(x + 180, y + i * 24 + 3, sig, 0xFFAAAA88, bg);
            fgl_draw_string(x + 280, y + i * 24 + 3, sec, 0xFF8888AA, bg);
        }
    }


    if (state->selected_ap != -1) {
        Rect conn_btn = {x, context_rect.y + context_rect.h - 40, 100, 24};
        fgl_fill_rect(conn_btn.x, conn_btn.y, conn_btn.w, conn_btn.h, 0xFF406040);
        fgl_draw_rect_border(conn_btn.x, conn_btn.y, conn_btn.w, conn_btn.h, 0xFF608060);
        fgl_draw_string(conn_btn.x + 12, conn_btn.y + 4, "Connect", 0xFFFFFFFF, 0x00000000);
    }
}

static void settings_on_draw(Window* win, void* app_data) {
    SettingsState* state = (SettingsState*)app_data;
    Rect r = win->rect;
    int sw = 140;


    fgl_fill_rect(r.x, r.y + TITLE_H, sw, r.h - TITLE_H, 0xFF14141A);
    fgl_draw_rect_border(r.x + sw - 1, r.y + TITLE_H, 1, r.h - TITLE_H, 0xFF303040);

    for (int i = 0; i < TAB_COUNT; i++) {
        uint32_t bg = (state->active_tab == i) ? 0xFF252535 : 0xFF14141A;
        uint32_t fg = (state->active_tab == i) ? 0xFFCCCCFF : 0xFF8888AA;
        fgl_fill_rect(r.x + 4, r.y + TITLE_H + 8 + i * 36, sw - 8, 32, bg);
        fgl_draw_string(r.x + 12, r.y + TITLE_H + 16 + i * 36, tab_names[i], fg, bg);
    }


    Rect context_rect = {r.x + sw, r.y + TITLE_H, r.w - sw, r.h - TITLE_H};
    fgl_fill_rect(context_rect.x, context_rect.y, context_rect.w, context_rect.h, 0xFF08080C);

    if (state->active_tab == TAB_WIFI) {
        draw_wifi_tab(win, state, context_rect);
    } else {
        fgl_draw_string(context_rect.x + 20, context_rect.y + 20, tab_names[state->active_tab], 0xFFFFFFFF, 0xFF08080C);
        fgl_draw_string(context_rect.x + 20, context_rect.y + 50, "This module is coming soon in v4.1", 0xFF666677, 0xFF08080C);
    }
}

static void settings_on_event(Window* win, GUI_Event* event, void* app_data) {
    SettingsState* state = (SettingsState*)app_data;
    Rect r = win->rect;
    int sw = 140;

    if (event->type == EVENT_MOUSE_CLICK) {
        int mx = event->mouse_pos.x;
        int my = event->mouse_pos.y;


        if (mx >= r.x && mx < r.x + sw) {
            int tab_idx = (my - (r.y + TITLE_H + 8)) / 36;
            if (tab_idx >= 0 && tab_idx < TAB_COUNT) {
                state->active_tab = (SettingsTab)tab_idx;
                win->dirty = 1;
            }
        }


        if (state->active_tab == TAB_WIFI) {
            int cx = mx - (r.x + sw);
            int cy = my - (r.y + TITLE_H);


            if (cx >= 20 && cx < 100 && cy >= 30 && cy < 54) {
                state->scanning = 1;
                win->dirty = 1;
                wifi_scan();
                state->scanning = 0;
                strcpy(state->connect_status, "Scan complete.");
            }


            if (cx >= 20 && cx < r.w - sw - 20 && cy >= 94 && cy < 94 + 8 * 24) {
                int idx = (cy - 94) / 24;
                if (idx >= 0 && idx < wifi_ap_count) {
                    state->selected_ap = idx;
                    win->dirty = 1;
                }
            }


            if (state->selected_ap != -1) {
                if (cx >= 20 && cx < 120 && cy >= r.h - TITLE_H - 40 && cy < r.h - TITLE_H - 16) {
                    strcpy(state->connect_status, "Connecting...");
                    win->dirty = 1;

                    wifi_connect(wifi_aps[state->selected_ap].ssid, "");
                    strcpy(state->connect_status, "Connection sent.");
                }
            }
        }
    }
}

static void settings_on_destroy(void* app_data) {
    kfree(app_data);
}

void module_main(void) {
    SettingsState* state = (SettingsState*)kmalloc(sizeof(SettingsState));
    state->active_tab = TAB_SYSTEM;
    state->scanning = 0;
    state->selected_ap = -1;
    strcpy(state->connect_status, "WiFi Ready.");

    AppInterface* app = (AppInterface*)kmalloc(sizeof(AppInterface));
    app->on_draw = settings_on_draw;
    app->on_event = settings_on_event;
    app->on_destroy = settings_on_destroy;
    app->app_data = state;

    Window* win = create_window((Rect){120, 80, 520, 380}, "System Settings", 0xFF08080C);
    win->app = app;
}




