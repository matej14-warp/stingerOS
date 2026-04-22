








typedef struct {
    int8_t  dx, dy, dz;
    uint8_t buttons;
} mouse_event_t;

extern volatile int mouse_x, mouse_y;

void mouse_init(void);
void mouse_poll_backends(void);
int  mouse_poll(mouse_event_t *ev);






