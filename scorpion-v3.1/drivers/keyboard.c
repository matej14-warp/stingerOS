/* scorpion OS - drivers/keyboard.c
   PS/2 keyboard driver with IRQ1 handler              */

#include "keyboard.h"
#include "../kernel/descriptor_tables.h"
#include "../kernel/terminal.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t p){uint8_t v;__asm__("inb %1,%0":"=a"(v):"Nd"(p));return v;}

/* Circular key buffer */
#define KEYBUF_SIZE 256
static char keybuf[KEYBUF_SIZE];
static volatile int kb_head=0, kb_tail=0;

static const char scancode_map[128] = {
    0,   27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.','/',
    0,   '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,  /* F1-F10 */
    0,0,                   /* numlock, scroll lock */
    0,0,0,'-',0,0,0,'+',0,0,0,0, /* keypad */
    0,0,0,0,0              /* misc */
};

static const char scancode_shift[128] = {
    0,   27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,   '|','Z','X','C','V','B','N','M','<','>','?',
    0,   '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,
    0,0,0,'-',0,0,0,'+',0,0,0,0,
    0,0,0,0,0
};

static int shift_down = 0;
static int caps_lock  = 0;

static void kb_irq_handler(registers_t *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    if(sc == 0x2A || sc == 0x36) { shift_down=1; return; }
    if(sc == 0xAA || sc == 0xB6) { shift_down=0; return; }
    if(sc == 0x3A) { caps_lock ^= 1; return; }

    if(sc & 0x80) return; /* key release */
    if(sc >= 128)  return;

    char c;
    int use_shift = shift_down;
    if(caps_lock && sc>=0x10 && sc<=0x32) use_shift ^= 1;
    c = use_shift ? scancode_shift[sc] : scancode_map[sc];
    if(!c) return;

    int next = (kb_head + 1) % KEYBUF_SIZE;
    if(next != kb_tail) {
        keybuf[kb_head] = c;
        kb_head = next;
    }
}

void keyboard_init(void) {
    /* Flush any stale bytes sitting in the 8042 output buffer */
    while(inb(0x64) & 0x01) inb(0x60);

    register_interrupt_handler(33, kb_irq_handler); /* IRQ1 = INT33 */
}

/* Block until a key is available */
char keyboard_getchar(void) {
    while(kb_head == kb_tail) {
        /* sti ensures interrupts are on, hlt sleeps until next IRQ fires */
        __asm__ volatile("sti; hlt");
    }
    char c = keybuf[kb_tail];
    kb_tail = (kb_tail + 1) % KEYBUF_SIZE;
    return c;
}

/* Non-blocking: returns 0 if no key */
char keyboard_poll(void) {
    if(kb_head == kb_tail) return 0;
    char c = keybuf[kb_tail];
    kb_tail = (kb_tail + 1) % KEYBUF_SIZE;
    return c;
}