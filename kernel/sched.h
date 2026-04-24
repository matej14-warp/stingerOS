#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include "descriptor_tables.h"

#define MAX_THREADS 64
#define THREAD_STACK_SIZE 8192

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_SLEEPING,
    THREAD_STATE_ZOMBIE
} thread_state_t;

typedef struct thread {
    uint32_t esp;          /* Saved stack pointer */
    uint32_t ebp;
    uint32_t eip;
    uint32_t id;
    thread_state_t state;
    uint8_t *stack;
    struct thread *next;
    char name[32];
    uint32_t sleep_until;
} thread_t;

void sched_init(void);
thread_t *thread_create(const char *name, void (*fn)(void*), void *arg);
void schedule(registers_t *regs);
void thread_exit(void);
void thread_sleep(uint32_t ms);

extern thread_t *current_thread;

#endif
