



thread_t *threads[MAX_THREADS];
thread_t *current_thread = NULL;
uint32_t  next_thread_id = 0;

void sched_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) threads[i] = NULL;
    thread_t *kt = (thread_t*)kmalloc(sizeof(thread_t));
    kt->id    = next_thread_id++;
    kt->state = THREAD_STATE_RUNNING;
    kt->stack = NULL;
    kt->sleep_until = 0;
    const char *kn = "kernel";
    int i = 0;
    for (; kn[i] && i < 31; i++) kt->name[i] = kn[i];
    kt->name[i] = 0;
    threads[0] = kt;
    current_thread = kt;
}

thread_t *thread_create(const char *name, void (*fn)(void*), void *arg) {
    int idx = -1;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (!threads[i]) { idx = i; break; }
    }
    if (idx == -1) return NULL;

    thread_t *t = (thread_t*)kmalloc(sizeof(thread_t));
    if (!t) return NULL;
    t->id    = next_thread_id++;
    t->state = THREAD_STATE_READY;
    t->sleep_until = 0;
    t->stack = (uint8_t*)kmalloc(THREAD_STACK_SIZE);
    if (!t->stack) { kfree(t); return NULL; }

    int i = 0;
    while (name[i] && i < 31) { t->name[i] = name[i]; i++; }
    t->name[i] = 0;



    uint32_t *stack_top = (uint32_t *)(t->stack + THREAD_STACK_SIZE);

    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = 0;

    registers_t *frame = (registers_t *)((uint8_t *)stack_top - sizeof(registers_t));




    frame->ss        = 0x10;
    frame->esp       = (uint32_t)stack_top;
    frame->eflags    = 0x202;
    frame->cs        = 0x08;
    frame->eip       = (uint32_t)fn;
    frame->err_code  = 0;
    frame->int_no    = 0;
    frame->eax       = 0;
    frame->ecx       = 0;
    frame->edx       = 0;
    frame->ebx       = 0;
    frame->esp_dummy = 0;
    frame->ebp       = 0;
    frame->esi       = 0;
    frame->edi       = 0;
    frame->ds        = 0x10;

    t->esp = (uint32_t)frame;
    threads[idx] = t;
    terminal_printf("[sched] thread %d '%s' created\n", t->id, t->name);
    return t;
}

void thread_exit(void) {
    __asm__ volatile("cli");
    if (current_thread) current_thread->state = THREAD_STATE_ZOMBIE;
    __asm__ volatile("sti; int $32");
    for (;;) __asm__ volatile("hlt");
}

void thread_sleep(uint32_t ms) {
    extern uint32_t get_ticks(void);
    if (!current_thread) return;
    current_thread->sleep_until = get_ticks() + ms;
    current_thread->state = THREAD_STATE_SLEEPING;
    __asm__ volatile("int $32");
}

registers_t* schedule(registers_t *regs) {
    if (!current_thread) return regs;

    extern uint32_t get_ticks(void);
    uint32_t now = get_ticks();


    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] && threads[i]->state == THREAD_STATE_SLEEPING) {
            if ((uint32_t)(now - threads[i]->sleep_until) < 0x80000000u)
                threads[i]->state = THREAD_STATE_READY;
        }
    }


    current_thread->esp = (uint32_t)regs;

    int cur_idx = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] == current_thread) { cur_idx = i; break; }
    }


    int next_idx = -1;
    for (int i = 1; i <= MAX_THREADS; i++) {
        int idx = (cur_idx + i) % MAX_THREADS;
        if (threads[idx] && threads[idx]->state == THREAD_STATE_READY) {
            next_idx = idx;
            break;
        }
    }


    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] && threads[i]->state == THREAD_STATE_ZOMBIE
            && threads[i] != current_thread) {
            if (threads[i]->stack) kfree(threads[i]->stack);
            kfree(threads[i]);
            threads[i] = NULL;
        }
    }

    if (next_idx == -1) return regs;

    if (current_thread->state == THREAD_STATE_RUNNING)
        current_thread->state = THREAD_STATE_READY;

    current_thread = threads[next_idx];
    current_thread->state = THREAD_STATE_RUNNING;

    return (registers_t *)current_thread->esp;
}




