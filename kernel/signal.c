/* scorpion OS - kernel/signal.c */
#include "signal.h"
#include "terminal.h"
#include <stdint.h>

sigstate_t g_sigstate;

void signal_raise(int sig) {
    if (sig < 1 || sig >= NSIG) return;
    g_sigstate.pending |= (1u << sig);
}

void signal_dispatch(void) {
    uint32_t mask = g_sigstate.pending;
    g_sigstate.pending = 0;
    for (int i = 1; i < NSIG; i++) {
        if (!(mask & (1u << i))) continue;
        void (*h)(int) = g_sigstate.handlers[i];
        if (h == SIG_IGN) continue;
        if (h == SIG_DFL) {
            /* Default: print message for fatal signals */
            if (i == SIGINT)  { terminal_writestring("^C\n"); continue; }
            if (i == SIGTERM) { terminal_writestring("[signal] SIGTERM received\n"); continue; }
            if (i == SIGKILL) { terminal_writestring("[signal] SIGKILL\n"); continue; }
            /* Other defaults: ignore */
            continue;
        }
        h(i);
    }
}

void (*signal_handler(int sig, void (*handler)(int)))(int) {
    if (sig < 1 || sig >= NSIG) return SIG_DFL;
    void (*old)(int) = g_sigstate.handlers[sig];
    g_sigstate.handlers[sig] = handler;
    return old;
}
