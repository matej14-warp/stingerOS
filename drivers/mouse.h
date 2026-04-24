/* scorpion OS - drivers/mouse.h */
#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>

#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

typedef struct {
    int8_t  dx, dy, dz;
    uint8_t buttons;
} mouse_event_t;

extern volatile int mouse_x, mouse_y;

void mouse_init(void);
void mouse_poll_backends(void);
int  mouse_poll(mouse_event_t *ev);  /* returns 1 if event available */

#endif /* MOUSE_H */
