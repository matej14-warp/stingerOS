#include "sched.h"
#include "kmalloc.h"
#include "terminal.h"
#include "serial_debug.h"

thread_t *threads[MAX_THREADS];
thread_t *current_thread = NULL;
uint32_t next_thread_id = 0;

void sched_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) threads[i] = NULL;
    
    /* Create idle/kernel thread */
    thread_t *kernel_thread = (thread_t*)kmalloc(sizeof(thread_t));
    kernel_thread->id = next_thread_id++;
    kernel_thread->state = THREAD_STATE_RUNNING;
    kernel_thread->stack = NULL; /* Uses current kernel stack */
    const char *kname = "kernel";
    int i = 0;
    for(; kname[i] && i < 31; i++) kernel_thread->name[i] = kname[i];
    kernel_thread->name[i] = 0;
    threads[0] = kernel_thread;
    current_thread = kernel_thread;
}

thread_t *thread_create(const char *name, void (*fn)(void*), void *arg) {
    int idx = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!threads[i]) { idx = i; break; }
    }
    if (idx == -1) return NULL;

    thread_t *t = (thread_t*)kmalloc(sizeof(thread_t));
    t->id = next_thread_id++;
    t->state = THREAD_STATE_READY;
    t->stack = (uint8_t*)kmalloc(THREAD_STACK_SIZE);
    
    int i=0; while(name[i] && i<31) { t->name[i] = name[i]; i++; } t->name[i]=0;

    /* Initialize stack for context switching */
    uint32_t *stack_ptr = (uint32_t *)(t->stack + THREAD_STACK_SIZE);

    *--stack_ptr = (uint32_t)arg;     /* Argument to function */
    *--stack_ptr = (uint32_t)thread_exit; /* Return address for function */
    *--stack_ptr = (uint32_t)fn;      /* Function entry point */
    
    /* Simulate pusha and other regs */
    *--stack_ptr = 0x202; /* EFLAGS: IF=1 */
    *--stack_ptr = 0;    /* EAX */
    *--stack_ptr = 0;    /* ECX */
    *--stack_ptr = 0;    /* EDX */
    *--stack_ptr = 0;    /* EBX */
    *--stack_ptr = 0;    /* ESP dummy */
    *--stack_ptr = 0;    /* EBP */
    *--stack_ptr = 0;    /* ESI */
    *--stack_ptr = 0;    /* EDI */
    *--stack_ptr = 0x10; /* DS */

    t->esp = (uint32_t)stack_ptr;
    threads[idx] = t;
    return t;
}

void thread_exit(void) {
    __asm__ volatile("cli");
    current_thread->state = THREAD_STATE_ZOMBIE;
    /* Force immediate reschedule */
    __asm__ volatile("int $32");
    for(;;);
}

void schedule(registers_t *regs) {
    if (!current_thread) return;

    /* Save current thread state */
    current_thread->esp = (uint32_t)regs; /* This assumes regs is on the stack we want to save */

    /* Find next ready thread */
    int current_idx = -1;
    for(int i=0; i<MAX_THREADS; i++) {
        if(threads[i] == current_thread) { current_idx = i; break; }
    }

    int next_idx = -1;
    for(int i=1; i<=MAX_THREADS; i++) {
        int idx = (current_idx + i) % MAX_THREADS;
        if(threads[idx] && threads[idx]->state == THREAD_STATE_READY) {
            next_idx = idx;
            break;
        }
        if(threads[idx] && threads[idx]->state == THREAD_STATE_RUNNING && idx != current_idx) {
            next_idx = idx;
            break;
        }
    }

    if(next_idx != -1) {
        if(current_thread->state == THREAD_STATE_RUNNING)
            current_thread->state = THREAD_STATE_READY;
        
        current_thread = threads[next_idx];
        current_thread->state = THREAD_STATE_RUNNING;

        /* Context switch: replace regs on stack with new thread's saved state */
        /* This is tricky without assembly assistance for true stack switching.
           A better way is to have the ISR common stub handle it.
           For now, we'll do a simple pointer update if possible, 
           but usually this requires modifying the 'regs' pointer in the caller. */
    }
}
