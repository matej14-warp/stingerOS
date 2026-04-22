








static void button_click_handler(Window* win, GUI_Event* event);
static void textbox_key_handler(Window* win, GUI_Event* event);
static void menu_item_select_handler(Window* win, GUI_Event* event);


static GUI_Context* gui_ctx = NULL;


static void window_event_handler(Window* win, GUI_Event* event) {

    if (event->type == EVENT_CLOSE) {
        win->alive = 0;
    }
}


static void button_click_handler(Window* win, GUI_Event* event) {


    if (win && win->widgets[0] && win->widgets[0]->type == WIDGET_TYPE_BUTTON) {
        Widget* button = win->widgets[0];
        if (button->data) {
            int* state = (int*)button->data;
            if (*state < 2) (*state)++; else *state = 0;

            draw_widget(win, button);
        }
    }
}


static void textbox_key_handler(Window* win, GUI_Event* event) {

    if (event->type == EVENT_KEY_PRESS) {



    }
}


static void menu_item_select_handler(Window* win, GUI_Event* event) {
    if (event->type == EVENT_MENU_ITEM_CLICK) {


        if (event->menu_item_id == 1) {
            strncpy(win->title, "Menu Item 1 Selected!", MAX_WINDOW_TITLE - 1);
            win->title[MAX_WINDOW_TITLE - 1] = 0;
        }
    }
}


void create_demo_window(GUI_Context* ctx) {
    Rect win_rect = {100, 100, 600, 400};
    Window* win = gui_create_window(ctx, win_rect, "GUI Library Demo", 0xFF2A2A2A);
    if (!win) return;

    win->event_handler = window_event_handler;


    Widget* label = create_label((Rect){20, 20, 560, 30}, "Welcome to the GUI Library!", 0xFFFFFFFF, 0xFF2A2A2A);
    add_widget_to_window(win, label);

    Widget* button = create_button((Rect){20, 70, 120, 40}, "Click Me", 0xFFFFFFFF, 0xFF0078D7, button_click_handler);
    add_widget_to_window(win, button);

    Widget* textbox = create_textbox((Rect){20, 130, 560, 30}, "Enter text here...", 0xFFFFFFFF, 0xFF333333);
    textbox->on_key = textbox_key_handler;
    add_widget_to_window(win, textbox);


    Widget* menu = create_menu((Rect){20, 190, 150, 28}, 0xFFFFFFFF, 0xFF404040);
    if(menu) {
        add_menu_item(menu, "Option 1", 1);
        add_menu_item(menu, "Option 2", 2);
        add_menu_item(menu, "Option 3", 3);
        menu->event_handler = menu_item_select_handler;
        add_widget_to_window(win, menu);
    }



}


void gui_demo_main(void) {
    gui_ctx = gui_init();
    if (!gui_ctx) {

        return;
    }

    create_demo_window(gui_ctx);





    gui_draw(gui_ctx);












    gui_shutdown(gui_ctx);
    gui_ctx = NULL;
}




