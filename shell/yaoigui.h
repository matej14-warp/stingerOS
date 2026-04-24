/* scorpion OS - shell/yaoigui.h */
#ifndef YAOIGUI_H
#define YAOIGUI_H

/* Entry point for the YaoiGUI compositor (shell/yaoigui.c) */
void __attribute__((noreturn)) yaoigui_run(void);

/* Called from descriptor_tables.c on CPU exception inside the GUI */
extern int g_in_yaoigui;
void yaoigui_crash_exit(void);

#endif /* YAOIGUI_H */
