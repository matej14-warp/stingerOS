/* scorpion OS - drivers/sound.c
   PC speaker driver via PIT channel 2.
   Port 0x61 bit 0 = gate, bit 1 = speaker enable.
   PIT channel 2 (ports 0x42/0x43) generates the tone frequency.     */

#include "sound.h"
#include "../kernel/terminal.h"
#include <stdint.h>

static inline void outb_s(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static inline uint8_t inb_s(uint16_t p) {
    uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v;
}

#define PIT_BASE_FREQ 1193180u

void sound_init(void) {
    /* Nothing to initialise for the PC speaker */
}

void sound_beep(uint32_t freq_hz, uint32_t ms) {
    if (!freq_hz) { sound_stop(); return; }
    uint32_t div = PIT_BASE_FREQ / freq_hz;

    /* Set PIT channel 2 to square wave mode */
    outb_s(0x43, 0xB6);  /* channel 2, lobyte/hibyte, square wave */
    outb_s(0x42, (uint8_t)(div & 0xFF));
    outb_s(0x42, (uint8_t)((div >> 8) & 0xFF));

    /* Enable speaker: set bits 0 and 1 of port 0x61 */
    uint8_t port61 = inb_s(0x61);
    outb_s(0x61, port61 | 0x03);

    /* Simple busy-wait delay (~1ms per inner loop at ~1GHz, rough) */
    for (uint32_t m = 0; m < ms; m++)
        for (volatile uint32_t i = 0; i < 8000; i++);

    sound_stop();
}

void sound_stop(void) {
    uint8_t port61 = inb_s(0x61);
    outb_s(0x61, port61 & ~0x03);
}

void sound_play_startup(void) {
    /* Short ascending two-tone chime */
    sound_beep(523, 80);   /* C5 */
    sound_beep(659, 80);   /* E5 */
    sound_beep(784, 120);  /* G5 */
}

/* bin command */
#include "../fs/sfs.h"
void bin_sound(char **argv, int argc, struct sfs_node *cwd) {
    (void)cwd;
    if (argc >= 3) {
        uint32_t freq = 0, ms = 200;
        for (int i=0; argv[1][i]; i++) freq = freq*10 + (uint32_t)(argv[1][i]-'0');
        for (int i=0; argv[2][i]; i++) ms   = ms  *10 + (uint32_t)(argv[2][i]-'0');
        sound_beep(freq, ms);
        terminal_printf("beep: %uHz for %ums\n", freq, ms);
    } else if (argc == 2) {
        /* Named tones */
        if (argv[1][0]=='s'&&argv[1][1]=='t') { sound_play_startup(); }
        else terminal_printf("usage: sound <freq_hz> <duration_ms>\n       sound startup\n");
    } else {
        terminal_printf("usage: sound <freq_hz> <duration_ms>\n       sound startup\n");
    }
}
