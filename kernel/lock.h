




typedef struct {
    volatile int lock;
    volatile int owner;
    volatile int count;
} spinlock_t;

extern uint8_t lapic_get_id(void);

static inline void spin_lock(spinlock_t *sl) {
    int my_id = (int)lapic_get_id() + 1;
    if (sl->owner == my_id) {
        sl->count++;
        return;
    }

    int expected = 0;
    while (!__atomic_compare_exchange_n(&sl->lock, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        expected = 0;
        __asm__ volatile ("pause");
    }

    sl->owner = my_id;
    sl->count = 1;
}

static inline void spin_unlock(spinlock_t *sl) {
    if (--sl->count == 0) {
        sl->owner = 0;
        __atomic_store_n(&sl->lock, 0, __ATOMIC_RELEASE);
    }
}

static inline uint32_t spin_lock_irqsave(spinlock_t *sl) {
    uint32_t flags;
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    spin_lock(sl);
    return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *sl, uint32_t flags) {
    spin_unlock(sl);
    __asm__ volatile (
        "pushl %0\n\t"
        "popfl"
        :
        : "r"(flags)
        : "memory"
    );
}








