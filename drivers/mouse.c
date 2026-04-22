





volatile int mouse_x = 0, mouse_y = 0;

static inline uint8_t inb_m(uint16_t p) {
    uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v;
}
static inline void outb_m(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static inline uint32_t get_ticks_m(void) {
    extern uint32_t get_ticks(void); return get_ticks();
}

static void kb_wait_write(void) {
    int i = 100000;
    while ((inb_m(0x64) & 2) && i-- > 0);
}
static void kb_wait_read(void) {
    int i = 100000;
    while (!(inb_m(0x64) & 1) && i-- > 0);
}

static void mouse_write(uint8_t val) {
    kb_wait_write(); outb_m(0x64, 0xD4);
    kb_wait_write(); outb_m(0x60, val);
}
static uint8_t mouse_read(void) {
    kb_wait_read(); return inb_m(0x60);
}
static uint8_t mouse_read_ack(void) {

    int i = 50000;
    while (!(inb_m(0x64) & 1) && i-- > 0);
    if (i <= 0) return 0;
    return inb_m(0x60);
}



static mouse_event_t  mb_buf[MOUSE_BUF];
static volatile int   mb_head = 0, mb_tail = 0;

static void mb_push(mouse_event_t ev) {
    int next = (mb_head + 1) % MOUSE_BUF;
    if (next != mb_tail) { mb_buf[mb_head] = ev; mb_head = next; }
}


static uint8_t pkt[4];
static int     pkt_phase = 0;
static int     has_scroll = 0;
static int     pkt_size   = 3;


static int      tap_active   = 0;
static int      tap_moved    = 0;
static int      tap_dx_acc   = 0;
static int      tap_dy_acc   = 0;
static uint32_t tap_start    = 0;



static void process_packet(void) {

    if (!(pkt[0] & 0x08)) return;

    mouse_event_t ev;
    ev.buttons = pkt[0] & 0x07;


    int x_sign = (pkt[0] & 0x10);
    int y_sign = (pkt[0] & 0x20);

    ev.dx = (int)pkt[1];
    if (x_sign) ev.dx -= 256;

    ev.dy = (int)pkt[2];
    if (y_sign) ev.dy -= 256;

    ev.dz = has_scroll ? (int8_t)pkt[3] : 0;


    int any_hw_btn = (ev.buttons != 0);
    if (!any_hw_btn) {

        if (ev.dx != 0 || ev.dy != 0) {
            if (!tap_active) {
                tap_active  = 1;
                tap_moved   = 0;
                tap_dx_acc  = 0;
                tap_dy_acc  = 0;
                tap_start   = get_ticks_m();
            } else {
                tap_dx_acc += (ev.dx < 0 ? -ev.dx : ev.dx);
                tap_dy_acc += (ev.dy < 0 ? -ev.dy : ev.dy);
                if (tap_dx_acc > TAP_MOVE_THRESH || tap_dy_acc > TAP_MOVE_THRESH)
                    tap_moved = 1;
            }
        } else {

            if (tap_active) {
                tap_active = 0;
                uint32_t now = get_ticks_m();
                uint32_t dur = now - tap_start;
                if (!tap_moved && dur < TAP_TIME_MS) {

                    mouse_event_t tap_dn = {0, 0, 0, MOUSE_BTN_LEFT};
                    mouse_event_t tap_up = {0, 0, 0, 0};
                    mb_push(tap_dn);
                    mb_push(tap_up);
                    return;
                }
            }
        }
    }

    mb_push(ev);
}

static void mouse_irq_inner(void) {
    if (!(inb_m(0x64) & 0x20)) return;
    uint8_t b = inb_m(0x60);
    pkt[pkt_phase++] = b;
    if (pkt_phase < pkt_size) return;
    pkt_phase = 0;
    process_packet();
}

static void mouse_irq(registers_t *regs) {
    (void)regs;
    mouse_irq_inner();
}


static void synaptics_try_init(void) {

    for (int i = 0; i < 4; i++) {
        mouse_write(0xE8); mouse_read_ack();
        mouse_write(0x00); mouse_read_ack();
    }
    mouse_write(0xE9);
    mouse_read_ack();
    uint8_t b0 = mouse_read();
    uint8_t b1 = mouse_read();
    uint8_t b2 = mouse_read();
    (void)b0; (void)b2;

    if (b1 == 0x47) {



        uint8_t mode = 0x00;
        for (int b = 3; b >= 0; b--) {
            mouse_write(0xE8); mouse_read_ack();
            mouse_write((mode >> (b*2)) & 0x03); mouse_read_ack();
        }
        mouse_write(0xF3); mouse_read_ack();
        mouse_write(0x14); mouse_read_ack();
    }
}



void mouse_init(void) {
    terminal_printf("[mouse] initializing PS/2...\n");

    kb_wait_write(); outb_m(0x64, 0xA8);


    kb_wait_write(); outb_m(0x64, 0x20);
    kb_wait_read();
    uint8_t status = inb_m(0x60);
    terminal_printf("[mouse] 8042 status: 0x%02x\n", status);

    status |= 0x02;
    status &= ~0x20;
    status |= 0x01;
    kb_wait_write(); outb_m(0x64, 0x60);
    kb_wait_write(); outb_m(0x60, status);


    mouse_write(0xFF);
    uint8_t ack = mouse_read_ack();
    uint8_t bat = mouse_read();
    uint8_t id = mouse_read();
    terminal_printf("[mouse] Reset: ACK 0x%02x, BAT 0x%02x, ID 0x%02x\n", ack, bat, id);




    mouse_write(0xF6); mouse_read_ack();


    mouse_write(0xF3); mouse_read_ack(); mouse_write(200); mouse_read_ack();
    mouse_write(0xF3); mouse_read_ack(); mouse_write(100); mouse_read_ack();
    mouse_write(0xF3); mouse_read_ack(); mouse_write(80);  mouse_read_ack();


    mouse_write(0xF2); mouse_read_ack();
    uint8_t id2 = mouse_read();
    terminal_printf("[mouse] Final ID: 0x%02x\n", id2);
    if (id2 == 3 || id2 == 4) { has_scroll = 1; pkt_size = 4; }


    mouse_write(0xF3); mouse_read_ack();
    mouse_write(100);  mouse_read_ack();


    mouse_write(0xE8); mouse_read_ack();
    mouse_write(0x03); mouse_read_ack();


    mouse_write(0xF4); mouse_read_ack();

    register_interrupt_handler(44, mouse_irq);
    terminal_printf("[mouse] initialization complete\n");
}


void mouse_poll_backends(void) {

    int limit = 512;
    while (limit-- > 0) {
        uint8_t st = inb_m(0x64);
        if (!(st & 0x01)) break;
        if (!(st & 0x20)) break;
        mouse_irq_inner();
    }
}

int mouse_poll(mouse_event_t *ev) {
    if (mb_head == mb_tail) return 0;
    *ev = mb_buf[mb_tail];
    mb_tail = (mb_tail + 1) % MOUSE_BUF;
    return 1;
}




