





























typedef struct {
    void (*handlers[NSIG])(int);
    uint32_t pending;
} sigstate_t;

extern sigstate_t g_sigstate;

void signal_raise(int sig);
void signal_dispatch(void);
void (*signal_handler(int sig, void (*handler)(int)))(int);






