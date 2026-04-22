








Widget* create_widget(WidgetType type, Rect rect, const char* text, uint32_t fg, uint32_t bg) {
    Widget* w = (Widget*)kmalloc(sizeof(Widget));
    if (!w) return NULL;

    w->type = type;
    w->rect = rect;
    w->fg_color = fg;
    w->bg_color = bg;
    w->next = NULL;
    w->data = NULL;
    w->on_click = NULL;
    w->on_key = NULL;
    w->on_paint = NULL;

    if (text) {
        w->text = (char*)kmalloc(strlen(text) + 1);
        if (!w->text) {
            kfree(w);
            return NULL;
        }
        strcpy(w->text, text);
    } else {
        w->text = NULL;
    }
    return w;
}

void destroy_widget(Widget* widget) {
    if (!widget) return;
    if (widget->text) kfree(widget->text);

    kfree(widget);
}


static void draw_label_widget(Window* win, Widget* widget) {
    fgl_fill_rect(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, widget->bg_color);
    if (widget->text) {

        fgl_draw_string(widget->rect.x + 2, widget->rect.y + (widget->rect.h - 16) / 2, widget->text, widget->fg_color, widget->bg_color);
    }
}

static void draw_button_widget(Window* win, Widget* widget) {

    uint32_t btn_bg = widget->bg_color;
    uint32_t btn_fg = widget->fg_color;


    if (widget->data) {
        int state = *(int*)widget->data;
        if (state == 1) {
            btn_bg = (btn_bg & 0xFF000000) | ((btn_bg & 0x00FFFFFF) + 0x101010);
            if (btn_bg > 0xFFFFFFFF) btn_bg = 0xFFFFFFFF;
        } else if (state == 2) {
            btn_bg = (btn_bg & 0xFF000000) | ((btn_bg & 0x00FFFFFF) - 0x101010);
            if (btn_bg < 0x303030) btn_bg = 0x303030;
        }
    }

    fgl_fill_rect(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, btn_bg);
    fgl_draw_rect_border(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, 0xFF404040);

    if (widget->text) {

        int text_width = strlen(widget->text) * 8;
        int text_x = widget->rect.x + (widget->rect.w - text_width) / 2;
        int text_y = widget->rect.y + (widget->rect.h - 16) / 2;
        fgl_draw_string(text_x, text_y, widget->text, btn_fg, btn_bg);
    }
}

static void draw_textbox_widget(Window* win, Widget* widget) {
    fgl_fill_rect(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, widget->bg_color);
    fgl_draw_rect_border(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, 0xFF606060);
    if (widget->text) {
        fgl_draw_string(widget->rect.x + 4, widget->rect.y + (widget->rect.h - 16) / 2, widget->text, widget->fg_color, widget->bg_color);
    }


    if (win->focused_window_idx != -1 && win->is_focused) {


        int cursor_x = widget->rect.x + 4 + strlen(widget->text) * 8;
        if (cursor_x < widget->rect.x + widget->rect.w - 4) {
            fgl_draw_rect_border(cursor_x, widget->rect.y + 4, 2, widget->rect.h - 8, widget->fg_color);
        }
    }
}


typedef struct MenuItem {
    char* text;
    int id;
    struct MenuItem* next;
} MenuItem;

typedef struct Menu {
    MenuItem* items;
    int item_count;
    int selected_item_idx;
    int is_open;
} Menu;

Widget* create_menu(Rect rect, uint32_t fg, uint32_t bg) {
    Widget* menu = create_widget(WIDGET_TYPE_MENU, rect, NULL, fg, bg);
    if (menu) {
        menu->data = kmalloc(sizeof(Menu));
        if (!menu->data) {
            destroy_widget(menu);
            return NULL;
        }
        Menu* m = (Menu*)menu->data;
        m->items = NULL;
        m->item_count = 0;
        m->selected_item_idx = -1;
        m->is_open = 0;
    }
    return menu;
}

void add_menu_item(Widget* menu_widget, const char* text, int id) {
    if (!menu_widget || menu_widget->type != WIDGET_TYPE_MENU) return;
    Menu* m = (Menu*)menu_widget->data;
    MenuItem* item = (MenuItem*)kmalloc(sizeof(MenuItem));
    if (!item) return;

    item->text = (char*)kmalloc(strlen(text) + 1);
    if (!item->text) {
        kfree(item);
        return;
    }
    strcpy(item->text, text);
    item->id = id;
    item->next = NULL;


    if (!m->items) {
        m->items = item;
    } else {
        MenuItem* current = m->items;
        while (current->next) {
            current = current->next;
        }
        current->next = item;
    }
    m->item_count++;
}

void destroy_menu(Widget* menu_widget) {
    if (!menu_widget || menu_widget->type != WIDGET_TYPE_MENU) return;
    Menu* m = (Menu*)menu_widget->data;
    if (m) {
        MenuItem* current = m->items;
        while (current) {
            MenuItem* next = current->next;
            if (current->text) kfree(current->text);
            kfree(current);
            current = next;
        }
        kfree(m);
    }
    destroy_widget(menu_widget);
}

void draw_menu_widget(Window* win, Widget* widget) {
    if (!win || !widget || widget->type != WIDGET_TYPE_MENU) return;
    Menu* m = (Menu*)widget->data;


    fgl_fill_rect(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, widget->bg_color);

    fgl_draw_rect_border(widget->rect.x, widget->rect.y, widget->rect.w, widget->rect.h, 0xFF606060);


    if (widget->text) {
        fgl_draw_string(widget->rect.x + 8, widget->rect.y + (widget->rect.h - 16) / 2, widget->text, widget->fg_color, widget->bg_color);
    }


    if (m && m->is_open) {
        int item_y = widget->rect.y + widget->rect.h;
        int item_h = 24;


        int menu_w = widget->rect.w;
        int menu_h = m->item_count * item_h + 4;
        if (menu_h < 40) menu_h = 40;
        Rect menu_rect = {widget->rect.x, item_y, menu_w, menu_h};
        fgl_fill_rect(menu_rect.x, menu_rect.y, menu_rect.w, menu_rect.h, widget->bg_color);
        fgl_draw_rect_border(menu_rect.x, menu_rect.y, menu_rect.w, menu_rect.h, 0xFF606060);

        for(MenuItem* item = m->items; item != NULL; item = item->next) {
            uint32_t item_bg = (item->id == m->selected_item_idx) ? C_ACCENT : widget->bg_color;
            uint32_t item_fg = (item->id == m->selected_item_idx) ? C_WHITE : widget->fg_color;
            fgl_fill_rect(menu_rect.x, item_y, menu_rect.w, item_h, item_bg);
            fgl_draw_string(menu_rect.x + 8, item_y + (item_h - 16) / 2, item->text, item_fg, item_bg);
            item_y += item_h;
        }
    }
}


void draw_widget(Window* win, Widget* widget) {
    if (!win || !widget) return;
    switch (widget->type) {
        case WIDGET_TYPE_LABEL:
            draw_label_widget(win, widget);
            break;
        case WIDGET_TYPE_BUTTON:
            draw_button_widget(win, widget);
            break;
        case WIDGET_TYPE_TEXTBOX:
            draw_textbox_widget(win, widget);
            break;
        case WIDGET_TYPE_MENU:
            draw_menu_widget(win, widget);
            break;
        default: break;
    }
}



Window* create_window(Rect rect, const char* title, uint32_t bg_color) {
    Window* win = (Window*)kmalloc(sizeof(Window));
    if (!win) return NULL;

    memset(win, 0, sizeof(Window));
    win->rect = rect;
    strncpy(win->title, title, MAX_WINDOW_TITLE - 1);
    win->title[MAX_WINDOW_TITLE - 1] = 0;
    win->background_color = bg_color;
    win->state = WINDOW_STATE_NORMAL;
    win->is_draggable = 1;
    win->is_resizable = 1;
    win->is_focused = 0;
    win->event_handler = NULL;
    win->widget_count = 0;
    win->is_dragging = 0;
    win->drag_offset.x = 0; win->drag_offset.y = 0;
    win->is_resizing = 0;
    win->resize_handle = -1;


    win->border_color_active = 0xFF0078D7;
    win->border_color_inactive = 0xFF333333;
    win->title_bar_color_active = 0xFF333333;
    win->title_bar_color_inactive = 0xFF2A2A2A;


    win->rendering_context = NULL;


    win->alive = 1;

    return win;
}

void destroy_window(Window* win) {
    if (!win) return;
    for (int i = 0; i < win->widget_count; i++) {
        destroy_widget(win->widgets[i]);
    }
    kfree(win);
}

void add_widget_to_window(Window* win, Widget* widget) {
    if (!win || !widget || win->widget_count >= MAX_WIDGETS_PER_WINDOW) return;
    win->widgets[win->widget_count++] = widget;

}

void set_window_focus(Window* win) {

    if (!win) return;
    win->is_focused = 1;

}



Rect get_window_resize_handle_rect(Window* win, int handle_idx) {
    Rect r = win->rect;
    int handle_size = 8;
    int corner_offset = handle_size / 2;

    switch(handle_idx) {
        case 0: return (Rect){r.x - corner_offset, r.y - corner_offset, handle_size, handle_size};
        case 1: return (Rect){r.x + r.w/2 - corner_offset, r.y - corner_offset, handle_size, handle_size};
        case 2: return (Rect){r.x + r.w - corner_offset, r.y - corner_offset, handle_size, handle_size};
        case 3: return (Rect){r.x - corner_offset, r.y + r.h/2 - corner_offset, handle_size, handle_size};
        case 4: return (Rect){r.x + r.w - corner_offset, r.y + r.h/2 - corner_offset, handle_size, handle_size};
        case 5: return (Rect){r.x - corner_offset, r.y + r.h - corner_offset, handle_size, handle_size};
        case 6: return (Rect){r.x + r.w/2 - corner_offset, r.y + r.h - corner_offset, handle_size, handle_size};
        case 7: return (Rect){r.x + r.w - corner_offset, r.y + r.h - corner_offset, handle_size, handle_size};
        default: return (Rect){0,0,0,0};
    }
}


int get_resize_handle(Window* win, Point mouse_pos) {
    if (!win || !win->is_resizable) return -1;

    Rect r = win->rect;
    int handle_size = 8;
    int border_thickness = 4;


    if (is_point_in_rect(mouse_pos, get_window_resize_handle_rect(win, 0))) return 0;
    if (is_point_in_rect(mouse_pos, get_window_resize_handle_rect(win, 2))) return 2;
    if (is_point_in_rect(mouse_pos, get_window_resize_handle_rect(win, 5))) return 5;
    if (is_point_in_rect(mouse_pos, get_window_resize_handle_rect(win, 7))) return 7;


    if (mouse_pos.y >= r.y && mouse_pos.y < r.y + border_thickness && mouse_pos.x >= r.x + border_thickness && mouse_pos.x < r.x + r.w - border_thickness) return 1;
    if (mouse_pos.y >= r.y + r.h - border_thickness && mouse_pos.y < r.y + r.h && mouse_pos.x >= r.x + border_thickness && mouse_pos.x < r.x + r.w - border_thickness) return 6;
    if (mouse_pos.x >= r.x && mouse_pos.x < r.x + border_thickness && mouse_pos.y >= r.y + border_thickness && mouse_pos.y < r.y + r.h - border_thickness) return 3;
    if (mouse_pos.x >= r.x + r.w - border_thickness && mouse_pos.x < r.x + r.w && mouse_pos.y >= r.y + border_thickness && mouse_pos.y < r.y + r.h - border_thickness) return 4;

    return -1;
}


void draw_window(Window* win) {
    if (!win || win->state == WINDOW_STATE_CLOSED) return;

    int screen_w = vbe_get_width();
    int screen_h = vbe_get_height();


    if (win->rect.x < 0) win->rect.x = 0;
    if (win->rect.y < 0) win->rect.y = 0;
    if (win->rect.x + win->rect.w > screen_w) win->rect.x = screen_w - win->rect.w;
    if (win->rect.y + win->rect.h > screen_h) win->rect.y = screen_h - win->rect.h;

    if (win->rect.w < 100) win->rect.w = 100;
    if (win->rect.h < TITLE_H + 20) win->rect.h = TITLE_H + 20;


    uint32_t border_color = win->is_focused ? win->border_color_active : win->border_color_inactive;
    uint32_t title_bar_color = win->is_focused ? win->title_bar_color_active : win->title_bar_color_inactive;
    uint32_t title_text_color = win->is_focused ? 0xFFFFFFFF : 0xFFCCCCCC;


    fgl_fill_rect(win->rect.x, win->rect.y, win->rect.w, win->rect.h, win->background_color);


    fgl_fill_rect(win->rect.x, win->rect.y, win->rect.w, TITLE_H, title_bar_color);

    if (win->title[0]) {
        fgl_draw_string(win->rect.x + 8, win->rect.y + (TITLE_H - 16) / 2, win->title, title_text_color, title_bar_color);
    }


    fgl_fill_rect(win->rect.x, win->rect.y, win->rect.w, 1, border_color);
    fgl_fill_rect(win->rect.x, win->rect.y + win->rect.h - 1, win->rect.w, 1, border_color);
    fgl_fill_rect(win->rect.x, win->rect.y, 1, win->rect.h, border_color);
    fgl_fill_rect(win->rect.x + win->rect.w - 1, win->rect.y, 1, win->rect.h, border_color);


    if (win->app && win->app->on_draw) {
        win->app->on_draw(win, win->app->app_data);
    }


    for (int i = 0; i < win->widget_count; i++) {
        draw_widget(win, win->widgets[i]);
    }
}

void handle_window_event(Window* win, GUI_Event* event) {
    if (!win || !win->alive) return;


    if (event->type == EVENT_DRAG && win->is_dragging) {
        win->rect.x = event->mouse_pos.x - win->drag_offset.x;
        win->rect.y = event->mouse_pos.y - win->drag_offset.y;

        if (win->rect.x < 0) win->rect.x = 0;
        if (win->rect.y < 0) win->rect.y = 0;
        if (win->rect.x + win->rect.w > vbe_get_width()) win->rect.x = vbe_get_width() - win->rect.w;
        if (win->rect.y + win->rect.h > vbe_get_height()) win->rect.y = vbe_get_height() - win->rect.h;
        return;
    }


    if (event->type == EVENT_MOUSE_MOVE && win->is_resizing) {
        int dx = event->mouse_pos.x - win->resize_start_pos.x;
        int dy = event->mouse_pos.y - win->resize_start_pos.y;

        Rect new_rect = win->resize_start_rect;



        if (win->resize_handle == 0) { new_rect.x = win->resize_start_rect.x + dx; new_rect.y = win->resize_start_rect.y + dy; new_rect.w = win->resize_start_rect.w - dx; new_rect.h = win->resize_start_rect.h - dy; }

        else if (win->resize_handle == 1) { new_rect.y = win->resize_start_rect.y + dy; new_rect.h = win->resize_start_rect.h - dy; }

        else if (win->resize_handle == 2) { new_rect.y = win->resize_start_rect.y + dy; new_rect.w = win->resize_start_rect.w + dx; new_rect.h = win->resize_start_rect.h - dy; }

        else if (win->resize_handle == 3) { new_rect.x = win->resize_start_rect.x + dx; new_rect.w = win->resize_start_rect.w - dx; }

        else if (win->resize_handle == 4) { new_rect.w = win->resize_start_rect.w + dx; }

        else if (win->resize_handle == 5) { new_rect.x = win->resize_start_rect.x + dx; new_rect.w = win->resize_start_rect.w - dx; new_rect.h = win->resize_start_rect.h + dy; }

        else if (win->resize_handle == 6) { new_rect.h = win->resize_start_rect.h + dy; }

        else if (win->resize_handle == 7) { new_rect.w = win->resize_start_rect.w + dx; new_rect.h = win->resize_start_rect.h + dy; }


        if (new_rect.w < 100) { new_rect.w = 100; if (win->resize_handle == 0 || win->resize_handle == 3 || win->resize_handle == 5) new_rect.x = win->rect.x + (win->rect.w - new_rect.w); }
        if (new_rect.h < TITLE_H + 20) { new_rect.h = TITLE_H + 20; if (win->resize_handle == 0 || win->resize_handle == 5 || win->resize_handle == 6) new_rect.y = win->rect.y + (win->rect.h - new_rect.h); }

        resize_window(win, new_rect);
        return;
    }
    if (event->type == EVENT_MOUSE_UP && win->is_resizing) {
        win->is_resizing = 0;
        win->resize_handle = -1;
        return;
    }


    if (event->type == EVENT_MOUSE_CLICK) {

        Rect close_btn_rect = {win->rect.x + win->rect.w - TITLE_BAR_BUTTON_SIZE, win->rect.y, TITLE_BAR_BUTTON_SIZE, TITLE_H};
        if (is_point_in_rect(event->mouse_pos, close_btn_rect)) {
            win->state = WINDOW_STATE_CLOSED;
            return;
        }
    }


    if (win->app && win->app->on_event) {
        win->app->on_event(win, event, win->app->app_data);
    }


    for (int i = 0; i < win->widget_count; i++) {
        Widget* w = win->widgets[i];
        if (is_point_in_rect(event->mouse_pos, w->rect)) {
            if (event->type == EVENT_MOUSE_CLICK && w->on_click) {
                w->on_click(win, event);
            }
        }
    }
}

void resize_window(Window* win, Rect new_rect) {
    if (!win) return;
    win->rect = new_rect;



}



GUI_Context* gui_init(void) {
    GUI_Context* ctx = (GUI_Context*)kmalloc(sizeof(GUI_Context));
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof(GUI_Context));
    ctx->focused_window_idx = -1;
    return ctx;
}

void gui_shutdown(GUI_Context* ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->window_count; i++) {
        if (ctx->windows[i]) destroy_window(ctx->windows[i]);
    }
    kfree(ctx);
}

Window* gui_create_window(GUI_Context* ctx, Rect rect, const char* title, uint32_t bg_color) {
    if (!ctx || ctx->window_count >= 16) return NULL;
    Window* win = create_window(rect, title, bg_color);
    if (!win) return NULL;
    ctx->windows[ctx->window_count++] = win;
    return win;
}

void gui_process_event(GUI_Context* ctx, GUI_Event* event) {
    if (!ctx) return;


    ctx->mouse_pos = event->mouse_pos;
    ctx->mouse_buttons = event->mouse_button;

    Window* target_win = NULL;
    int target_idx = -1;



    if (ctx->focused_window_idx != -1 && ctx->windows[ctx->focused_window_idx] && ctx->windows[ctx->focused_window_idx]->alive) {
        target_win = ctx->windows[ctx->focused_window_idx];
        target_idx = ctx->focused_window_idx;
    }


    if (!target_win || event->type == EVENT_MOUSE_CLICK || event->type == EVENT_MOUSE_MOVE || event->type == EVENT_MOUSE_DOWN || event->type == EVENT_MOUSE_UP) {
        for (int i = ctx->window_count - 1; i >= 0; i--) {
            if (ctx->windows[i] && ctx->windows[i]->alive && ctx->windows[i]->state != WINDOW_STATE_MINIMIZED) {
                if (is_point_in_rect(event->mouse_pos, ctx->windows[i]->rect)) {

                    if ((event->type == EVENT_MOUSE_DOWN || event->type == EVENT_MOUSE_MOVE) && ctx->windows[i]->is_resizable) {
                        int handle = get_resize_handle(ctx->windows[i], event->mouse_pos);
                        if (handle != -1) {
                            ctx->windows[i]->is_resizing = 1;
                            ctx->windows[i]->resize_handle = handle;
                            ctx->windows[i]->resize_start_pos = event->mouse_pos;
                            ctx->windows[i]->resize_start_rect = ctx->windows[i]->rect;
                            target_win = ctx->windows[i];
                            target_idx = i;
                            break;
                        }
                    }

                    if (event->type == EVENT_MOUSE_DOWN && !ctx->windows[i]->is_resizing && (event->mouse_pos.y >= ctx->windows[i]->rect.y && event->mouse_pos.y < ctx->windows[i]->rect.y + TITLE_H)) {
                        if (ctx->windows[i]->is_draggable) {
                            ctx->windows[i]->is_dragging = 1;
                            ctx->windows[i]->drag_offset.x = event->mouse_pos.x - ctx->windows[i]->rect.x;
                            ctx->windows[i]->drag_offset.y = event->mouse_pos.y - ctx->windows[i]->rect.y;
                            target_win = ctx->windows[i];
                            target_idx = i;
                            break;
                        }
                    }

                    if (event->type == EVENT_MOUSE_CLICK) {
                        target_win = ctx->windows[i];
                        target_idx = i;
                        break;
                    }
                }
            }
        }
    }

    if (target_win) {

        if (ctx->focused_window_idx != target_idx) {
            if (ctx->focused_window_idx != -1 && ctx->windows[ctx->focused_window_idx]) {
                ctx->windows[ctx->focused_window_idx]->is_focused = 0;
            }
            target_win->is_focused = 1;
            ctx->focused_window_idx = target_idx;


            if (target_idx != ctx->window_count - 1) {
                Window* temp = ctx->windows[target_idx];
                for (int i = target_idx; i < ctx->window_count - 1; i++) {
                    ctx->windows[i] = ctx->windows[i+1];
                }
                ctx->windows[ctx->window_count - 1] = temp;
                ctx->focused_window_idx = ctx->window_count - 1;
            }
        }


        if (event->type == EVENT_MOUSE_UP) {
            target_win->is_dragging = 0;
            target_win->is_resizing = 0;
            target_win->resize_handle = -1;
        }

        handle_window_event(target_win, event);
    } else {


        if (event->type == EVENT_MOUSE_CLICK) {
             if (ctx->focused_window_idx != -1 && ctx->windows[ctx->focused_window_idx]) {
                ctx->windows[ctx->focused_window_idx]->is_focused = 0;
             }
             ctx->focused_window_idx = -1;
        }
    }
}

void gui_draw(GUI_Context* ctx) {
    if (!ctx) return;

    fgl_fill_rect(0, 0, vbe_get_width(), vbe_get_height(), 0xFF1A1A2E);


    for (int i = 0; i < ctx->window_count; i++) {
        if (ctx->windows[i] && ctx->windows[i]->alive) {
            draw_window(ctx->windows[i]);
        }
    }

}

int is_point_in_rect(Point p, Rect r) {
    return p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h;
}




