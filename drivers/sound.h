




void sound_init(void);
void sound_beep(uint32_t freq_hz, uint32_t ms);
void sound_play_startup(void);
void sound_stop(void);


void sound_set_volume(int vol);
int  sound_get_volume(void);
void sound_toggle_mute(void);


void hda_init(void);


struct sfs_node;
void bin_sound(char **argv, int argc, struct sfs_node *cwd);






