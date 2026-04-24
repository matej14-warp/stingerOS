/* scorpion OS - shell/games.h */
#ifndef GAMES_H
#define GAMES_H

/* terminal games */
void cmd_snake(void);
void cmd_tetris(void);
void cmd_pong(void);
void cmd_sysinfo(void);
void cmd_memtest(void);
void cmd_cpubench(void);

/* GUI (VBE framebuffer) games — called from yaoigui WAPP_SNAKE/TETRIS/PONG windows */
void cmd_snake_gui(int wx, int wy, int ww, int wh);
void cmd_tetris_gui(int wx, int wy, int ww, int wh);
void cmd_pong_gui(int wx, int wy, int ww, int wh);

/* Minecraft raycaster — called from yaoigui WAPP_MINECRAFT window */
void cmd_minecraft_gui(int wx, int wy, int ww, int wh);

#endif /* GAMES_H */
