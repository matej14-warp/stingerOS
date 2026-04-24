/* scorpion OS - drivers/mouse.c
   PS/2 mouse driver via 8042 auxiliary port.
   v2: improved touchpad support.
   • IntelliMouse scroll wheel (4-byte packets) enabled.
   • Synaptics relative-mode init to make integrated touchpads work.
   • Aggressive polling fallback: drains ALL pending mouse bytes per
     call so a high-rate touchpad (typically 100–200 reports/sec) does
     not stall behind a 16-byte read limit.
   • Larger ring buffer (256 entries) to absorb bursts.
   • Touchpad tap-to-click: left-click synthesised on finger-up after a
     short tap (< 150 ms, < 5 pixel movement).
   Events queued in ring buffer; mouse_poll() dequeues one at a time.  */

#include "mouse.h"
#include "../kernel/descriptor_tables.h"
#include <stdint.h>

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
    /* read with timeout: returns 0xFA (ACK) or 0xFE (NACK) or 0 on timeout */
    int i = 50000;
    while (!(inb_m(0x64) & 1) && i-- > 0);
    if (i <= 0) return 0;
    return inb_m(0x60);
}

/* ================================================================ EVENT RING */
#define MOUSE_BUF 256
static mouse_event_t  mb_buf[MOUSE_BUF];
static volatile int   mb_head = 0, mb_tail = 0;

static void mb_push(mouse_event_t ev) {
    int next = (mb_head + 1) % MOUSE_BUF;
    if (next != mb_tail) { mb_buf[mb_head] = ev; mb_head = next; }
}

/* ================================================================ PACKET ACCUMULATOR */
static uint8_t pkt[4];
static int     pkt_phase = 0;
static int     has_scroll = 0;
static int     pkt_size   = 3;

/* ================================================================ TAP-TO-CLICK STATE */
static int      tap_active   = 0;   /* finger is on pad */
static int      tap_moved    = 0;   /* has moved > threshold */
static int      tap_dx_acc   = 0;
static int      tap_dy_acc   = 0;
static uint32_t tap_start    = 0;
#define TAP_MOVE_THRESH  8   /* pixels — beyond this it's a drag */
#define TAP_TIME_MS    150   /* max tap duration */

static void process_packet(void) {
    /* Validate: bit 3 of byte 0 must be set */
    if (!(pkt[0] & 0x08)) return;

    mouse_event_t ev;
    ev.buttons = pkt[0] & 0x07;

    /* Sign-extend dx/dy using overflow bits from byte 0 */
    ev.dx = (int8_t)pkt[1];
    if (pkt[0] & 0x10) ev.dx = (int8_t)(ev.dx | 0x00); /* negative already */

    ev.dy = (int8_t)pkt[2];
    ev.dz = has_scroll ? (int8_t)(pkt[3] & 0x0F) : 0;

    /* ---- tap-to-click state machine ---- */
    int any_hw_btn = (ev.buttons != 0);
    if (!any_hw_btn) {
        /* no physical button — trackpoint / touchpad move-only reports */
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
            /* Zero delta — finger lifted */
            if (tap_active) {
                tap_active = 0;
                uint32_t now = get_ticks_m();
                uint32_t dur = now - tap_start;
                if (!tap_moved && dur < TAP_TIME_MS) {
                    /* Synthesise a left-click press + release */
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

/* ================================================================ SYNAPTICS INIT
   Synaptics touchpads respond to the PS/2 "get info" command (0xE9 / 0xE8
   magic sequence) with a device ID of 0x47 in the second status byte.
   We put them in relative mode (mode byte = 0x00) so they behave exactly
   like a normal PS/2 mouse from the packet-parsing perspective.           */
static void synaptics_try_init(void) {
    /* Knock sequence: set resolution 0xE8 0x00 four times, then 0xE9 */
    for (int i = 0; i < 4; i++) {
        mouse_write(0xE8); mouse_read_ack();
        mouse_write(0x00); mouse_read_ack();
    }
    mouse_write(0xE9);  /* status request */
    mouse_read_ack();   /* ACK */
    uint8_t b0 = mouse_read(); /* info[0] */
    uint8_t b1 = mouse_read(); /* info[1] — model/capability */
    uint8_t b2 = mouse_read(); /* info[2] — modes */
    (void)b0; (void)b2;

    if (b1 == 0x47) {
        /* Confirmed Synaptics — put into relative mode */
        /* Mode byte: set bit 7 = 0 (relative), bit 6 = 0 (no W mode) */
        /* Use set-mode sequence: 0xE8 <mode_bits[7:6]> 0xE8 <mode_bits[5:4]>
                                  0xE8 <mode_bits[3:2]> 0xE8 <mode_bits[1:0]>
           then 0xF3 0x14 (set sample rate 20, triggers mode latch)       */
        uint8_t mode = 0x00;
        for (int b = 3; b >= 0; b--) {
            mouse_write(0xE8); mouse_read_ack();
            mouse_write((mode >> (b*2)) & 0x03); mouse_read_ack();
        }
        mouse_write(0xF3); mouse_read_ack();
        mouse_write(0x14); mouse_read_ack();  /* sample rate 20 → latches mode */
    }
}

/* ================================================================ INIT */
void mouse_init(void) {
    /* Enable auxiliary device */
    kb_wait_write(); outb_m(0x64, 0xA8);
    /* Enable mouse interrupt (bit 1 of command byte) */
    kb_wait_write(); outb_m(0x64, 0x20);
    kb_wait_read();
    uint8_t status = inb_m(0x60);
    status |= 0x02; status &= ~0x20;
    kb_wait_write(); outb_m(0x64, 0x60);
    kb_wait_write(); outb_m(0x60, status);

    /* Reset mouse and read ID */
    mouse_write(0xFF);
    mouse_read_ack();   /* ACK */
    mouse_read();       /* BAT result (0xAA) */
    uint8_t id0 = mouse_read(); /* device ID */
    (void)id0;

    /* Try Synaptics relative-mode handshake before IntelliMouse */
    synaptics_try_init();

    /* Set defaults */
    mouse_write(0xF6); mouse_read_ack();

    /* Try IntelliMouse (scroll wheel): set sample rate 200,100,80 */
    mouse_write(0xF3); mouse_read_ack(); mouse_write(200); mouse_read_ack();
    mouse_write(0xF3); mouse_read_ack(); mouse_write(100); mouse_read_ack();
    mouse_write(0xF3); mouse_read_ack(); mouse_write(80);  mouse_read_ack();

    /* Read device ID */
    mouse_write(0xF2); mouse_read_ack();
    uint8_t id = mouse_read();
    if (id == 3 || id == 4) { has_scroll = 1; pkt_size = 4; }

    /* Set sample rate to 100 reports/sec for smooth touchpad */
    mouse_write(0xF3); mouse_read_ack();
    mouse_write(100);  mouse_read_ack();

    /* Set resolution: 8 counts/mm (0x03) */
    mouse_write(0xE8); mouse_read_ack();
    mouse_write(0x03); mouse_read_ack();

    /* Enable data reporting */
    mouse_write(0xF4); mouse_read_ack();

    register_interrupt_handler(44, mouse_irq);  /* IRQ12 → vector 44 */
}

/* ================================================================ POLL BACKENDS
   Drain ALL pending mouse bytes from the 8042 output buffer.
   Called from the GUI main loop when IRQ12 is not available (many
   laptops). Previously limited to 16 bytes which was far too small for
   a Synaptics pad reporting 100 pkt/s at ~3-4 bytes each.              */
void mouse_poll_backends(void) {
    /* No hard limit — drain until buffer empty or 512 bytes consumed
       (prevents runaway if 8042 is stuck asserting OBF).               */
    int limit = 512;
    while (limit-- > 0) {
        uint8_t st = inb_m(0x64);
        if (!(st & 0x01)) break;   /* output buffer empty */
        if (!(st & 0x20)) break;   /* keyboard data — leave for IRQ1 */
        mouse_irq_inner();
    }
}

int mouse_poll(mouse_event_t *ev) {
    if (mb_head == mb_tail) return 0;
    *ev = mb_buf[mb_tail];
    mb_tail = (mb_tail + 1) % MOUSE_BUF;
    return 1;
}
