/* scorpion OS - drivers/sound.h */
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

void sound_init(void);
void sound_beep(uint32_t freq_hz, uint32_t ms);
void sound_play_startup(void);
void sound_stop(void);

/* bin/sound.c command */
struct sfs_node;
void bin_sound(char **argv, int argc, struct sfs_node *cwd);

#endif /* SOUND_H */
