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

static int e0 = 0;
static int ctrl_down = 0;

static void kb_push(char c) {
    int next = (kb_head + 1) % KEYBUF_SIZE;
    if (next != kb_tail) { keybuf[kb_head] = c; kb_head = next; }
}

static void kb_irq_handler(registers_t *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) { e0 = 1; return; }

    if (e0) {
        e0 = 0;
        int released = (sc & 0x80) != 0;
        sc &= 0x7F;
        if (!released) {
            switch (sc) {
            case 0x48: kb_push('\x1b'); kb_push('['); kb_push('A'); return;
            case 0x50: kb_push('\x1b'); kb_push('['); kb_push('B'); return;
            case 0x4D: kb_push('\x1b'); kb_push('['); kb_push('C'); return;
            case 0x4B: kb_push('\x1b'); kb_push('['); kb_push('D'); return;
            case 0x47: kb_push('\x1b'); kb_push('['); kb_push('H'); return;
            case 0x4F: kb_push('\x1b'); kb_push('['); kb_push('F'); return;
            case 0x49: kb_push('\x1b'); kb_push('['); kb_push('5'); kb_push('~'); return;
            case 0x51: kb_push('\x1b'); kb_push('['); kb_push('6'); kb_push('~'); return;
            case 0x53: kb_push('\x1b'); kb_push('['); kb_push('3'); kb_push('~'); return;
            case 0x1C: kb_push('\n'); return;
            case 0x5B: kb_push('\x02'); return; /* Left Windows key -> STX */
            case 0x5C: kb_push('\x02'); return; /* Right Windows key -> STX */
            }
        }
        return;
    }

    if(sc == 0x2A || sc == 0x36) { shift_down=1; return; }
    if(sc == 0xAA || sc == 0xB6) { shift_down=0; return; }
    if(sc == 0x1D) { ctrl_down=1; return; }
    if(sc == 0x9D) { ctrl_down=0; return; }
    if(sc == 0x3A) { caps_lock ^= 1; return; }

    if(sc & 0x80) return;
    if(sc >= 128)  return;

    char c;
    int use_shift = shift_down;
    if(caps_lock && sc>=0x10 && sc<=0x32) use_shift ^= 1;
    c = use_shift ? scancode_shift[sc] : scancode_map[sc];
    if(!c) return;

    if(ctrl_down) {
        if(c >= 'a' && c <= 'z') c = (char)(c - 'a' + 1);
        else if(c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 1);
        else if(c == '[') c = '\x1b';
    }

    kb_push(c);
}

static inline void outb_kb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static void kb_wait_write(void) {
    int i = 100000;
    while ((inb(0x64) & 2) && i-- > 0);
}

void keyboard_init(void) {
    /* Flush stale bytes */
    while (inb(0x64) & 0x01) inb(0x60);

    /* Ensure the 8042 controller has translation enabled (bit 6 of CCB).
       This translates set-2 scancodes from the keyboard into set-1.
       We only touch the CONTROLLER config, never the keyboard device itself.
       Sending commands to the keyboard (0xF0 etc.) can break real hardware. */
    kb_wait_write();
    outb_kb(0x64, 0x20);                    /* read controller config byte */
    int i = 100000;
    while (!(inb(0x64) & 0x01) && i-- > 0); /* wait for response */
    uint8_t ccb = inb(0x60);
    ccb |= (1 << 6);                         /* bit 6: enable translation */
    ccb &= ~(1 << 4);                        /* bit 4: enable keyboard port */
    ccb |= (1 << 0);                         /* bit 0: enable IRQ1 */
    kb_wait_write();
    outb_kb(0x64, 0x60);                    /* write controller config byte */
    kb_wait_write();
    outb_kb(0x60, ccb);

    /* Flush again in case the CCB read put something in the buffer */
    while (inb(0x64) & 0x01) inb(0x60);

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

int keyboard_keyready(void) {
    return kb_head != kb_tail;
}

/* Peek at next char without consuming it. Returns 0 if none. */
char keyboard_peek(void) {
    if(kb_head == kb_tail) return 0;
    return keybuf[kb_tail];
}

char keyboard_getchar_timeout(uint32_t ms) {
    extern uint32_t get_ticks(void);
    uint32_t start = get_ticks();
    while ((uint32_t)(get_ticks() - start) < ms) {
        if (kb_head != kb_tail) return keyboard_getchar();
        __asm__ volatile("hlt");
    }
    return 0;
}