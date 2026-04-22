







static inline void outb_s(uint16_t p, uint8_t v)  { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t  inb_s(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }




static int g_volume = 75;
static int g_muted  = 0;


void sound_init(void) {
    g_volume = 75;
    g_muted  = 0;
    hda_init();
    if (hda_available())
        terminal_printf("[sound] HDA backend active\n");
    else
        terminal_printf("[sound] HDA not found — PC speaker only\n");
}


void sound_set_volume(int vol) {
    if (vol < 0)   vol = 0;
    if (vol > 100) vol = 100;
    g_volume = vol;

    if (hda_available())
        hda_set_volume((uint8_t)((g_volume * 255) / 100));
    terminal_printf("[sound] volume: %d%%\n", g_volume);
}

int sound_get_volume(void) { return g_muted ? 0 : g_volume; }

void sound_toggle_mute(void) {
    g_muted = !g_muted;
    if (hda_available())
        hda_set_volume(g_muted ? 0 : (uint8_t)((g_volume * 255) / 100));
    terminal_printf("[sound] %s\n", g_muted ? "muted" : "unmuted");
}


void sound_beep(uint32_t freq_hz, uint32_t ms) {
    if (!freq_hz || g_muted || g_volume == 0) { sound_stop(); return; }
    uint32_t div = PIT_BASE_FREQ / freq_hz;
    outb_s(0x43, 0xB6);
    outb_s(0x42, (uint8_t)(div & 0xFF));
    outb_s(0x42, (uint8_t)((div >> 8) & 0xFF));
    uint8_t p61 = inb_s(0x61);
    outb_s(0x61, p61 | 0x03);
    extern void thread_sleep(uint32_t ms);
    thread_sleep(ms);
    sound_stop();
}

void sound_stop(void) {
    uint8_t p61 = inb_s(0x61);
    outb_s(0x61, p61 & ~0x03u);
    if (hda_available()) hda_stop();
}

void sound_play_startup(void) {
    sound_beep(523, 80);
    sound_beep(659, 80);
    sound_beep(784, 120);
}



void bin_sound(char **argv, int argc, struct sfs_node *cwd) {
    (void)cwd;
    if (argc >= 3) {
        uint32_t freq = 0, ms = 200;
        for (int i = 0; argv[1][i]; i++) freq = freq * 10 + (uint32_t)(argv[1][i] - '0');
        for (int i = 0; argv[2][i]; i++) ms   = ms   * 10 + (uint32_t)(argv[2][i] - '0');
        sound_beep(freq, ms);
        terminal_printf("beep: %uHz for %ums\n", freq, ms);
    } else if (argc == 2) {
        if (argv[1][0] == 's' && argv[1][1] == 't') {
            sound_play_startup();
        } else if (argv[1][0] == 'v') {
            terminal_printf("volume: %d%%  hda: %s\n",
                g_volume, hda_available() ? "yes" : "no (pc speaker only)");
        } else {
            terminal_printf("usage: sound <freq_hz> <duration_ms>\n");
            terminal_printf("       sound startup\n");
            terminal_printf("       sound v          (show status)\n");
        }
    } else {
        terminal_printf("usage: sound <freq_hz> <duration_ms>\n");
        terminal_printf("       sound startup\n");
        terminal_printf("       sound v\n");
    }
}




