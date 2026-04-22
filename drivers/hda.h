






extern volatile uint8_t *g_hda_base;


void hda_isr(registers_t *regs);


void hda_init(void);


void hda_play_pcm(const int16_t *data, uint32_t n_frames);


void hda_stop(void);


void hda_set_volume(uint8_t vol);


int  hda_available(void);






