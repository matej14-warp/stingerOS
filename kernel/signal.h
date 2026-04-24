/* scorpion OS - kernel/signal.h + signal.c (combined via header guard trick)
   POSIX-like signals for the single-process kernel environment.
   Since there are no processes, signals are delivered to registered handlers
   and dispatched from the main shell loop.                                    */
#ifndef SIGNAL_H
#define SIGNAL_H
#include <stdint.h>

#define NSIG      32
#define SIG_DFL   ((void (*)(int))0)
#define SIG_IGN   ((void (*)(int))1)

/* Standard signal numbers */
#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20

typedef struct {
    void (*handlers[NSIG])(int);
    uint32_t pending;   /* bitmask of pending signals */
} sigstate_t;

extern sigstate_t g_sigstate;

void signal_raise(int sig);
void signal_dispatch(void);   /* called from main loop to deliver pending */
void (*signal_handler(int sig, void (*handler)(int)))(int);

#endif /* SIGNAL_H */
