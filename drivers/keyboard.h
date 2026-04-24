/* scorpion OS - drivers/keyboard.h */
#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

void keyboard_init(void);
char keyboard_getchar(void);         /* blocking */
char keyboard_poll(void);            /* non-blocking, consumes key, returns 0 if none */
char keyboard_peek(void);            /* non-blocking, does NOT consume, returns 0 if none */
int  keyboard_keyready(void);        /* 1 if a key is waiting */
char keyboard_getchar_timeout(uint32_t ms); /* returns 0 on timeout */

#endif /* KEYBOARD_H */
