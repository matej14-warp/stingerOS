






static inline uint8_t inb(uint16_t p) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p));
    return v;
}
static inline void outb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p));
}






static char keybuf[KEYBUF_SIZE];
static volatile int kb_head=0, kb_tail=0;

static const char scancode_map[128] = {
    0,   27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.','/',
    0,   '*', 0, ' ', 0,
    '\x11','\x12','\x13','\x14','\x15','\x16','\x17','\x18','\x19','\x1a',
    0,0,
    0,0,0,'-',0,0,0,'+',0,0,0,0,
    0,0,0,0,0
};

static const char scancode_shift[128] = {
    0,   27, '!','@','
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,   '|','Z','X','C','V','B','N','M','<','>','?',
    0,   '*', 0, ' ', 0,
    '\x11','\x12','\x13','\x14','\x15','\x16','\x17','\x18','\x19','\x1a',
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
            case 0x5B: kb_push('\x02'); return;
            case 0x5C: kb_push('\x02'); return;
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

static void kb_wait_write(void) {
    int i = 100000;
    while ((inb(0x64) & 2) && i-- > 0);
}

void keyboard_init(void) {

    while (inb(0x64) & 0x01) inb(0x60);


    kb_wait_write();
    outb_kb(0x64, 0x20);
    int i = 100000;
    while (!(inb_m(0x64) & 0x01) && i-- > 0);
    uint8_t ccb = inb_m(0x60);


    ccb |= (1 << 6);
    ccb &= ~(1 << 4);
    ccb |= (1 << 0);


    kb_wait_write();
    outb_kb(0x64, 0x60);
    kb_wait_write();
    outb_kb(0x60, ccb);


    kb_wait_write();
    outb(0x60, 0xF4);


    while (inb(0x64) & 0x01) inb(0x60);

    register_interrupt_handler(33, kb_irq_handler);
}


char keyboard_getchar(void) {
    while(kb_head == kb_tail) {

        __asm__ volatile("sti; hlt");
    }
    char c = keybuf[kb_tail];
    kb_tail = (kb_tail + 1) % KEYBUF_SIZE;
    return c;
}


char keyboard_poll(void) {
    if(kb_head == kb_tail) return 0;
    char c = keybuf[kb_tail];
    kb_tail = (kb_tail + 1) % KEYBUF_SIZE;
    return c;
}

int keyboard_keyready(void) {
    return kb_head != kb_tail;
}


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



