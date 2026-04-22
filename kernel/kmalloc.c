





static spinlock_t heap_lock = SPINLOCK_INIT;






static uint8_t  g_heap[HEAP_SIZE] __attribute__((aligned(16)));
static int      g_heap_ready = 0;

typedef struct blk {
    uint32_t size;
    uint32_t magic;
} blk_t;

static blk_t *heap_start(void) { return (blk_t *)g_heap; }
static blk_t *next_blk(blk_t *b) {
    return (blk_t *)((uint8_t *)b + HDR_SIZE + b->size);
}
static int in_heap(blk_t *b) {
    return (uint8_t *)b >= g_heap &&
           (uint8_t *)b + HDR_SIZE <= g_heap + HEAP_SIZE;
}

static void heap_init(void) {
    blk_t *first = heap_start();
    first->size  = HEAP_SIZE - HDR_SIZE;
    first->magic = MAGIC_FREE;
    g_heap_ready = 1;
}

void *kmalloc(size_t size) {
    if (!size) return NULL;
    uint32_t flags = spin_lock_irqsave(&heap_lock);
    if (!g_heap_ready) heap_init();


    size = (size + ALIGN - 1) & ~(size_t)(ALIGN - 1);
    if (size < ALIGN) size = ALIGN;

    blk_t *b = heap_start();
    while (in_heap(b)) {
        if (b->magic == MAGIC_FREE && b->size >= size) {

            if (b->size >= size + HDR_SIZE + ALIGN) {
                blk_t *rest = (blk_t *)((uint8_t *)b + HDR_SIZE + size);
                rest->size  = b->size - size - HDR_SIZE;
                rest->magic = MAGIC_FREE;
                b->size     = (uint32_t)size;
            }
            b->magic = MAGIC_USED;
            spin_unlock_irqrestore(&heap_lock, flags);
            return (uint8_t *)b + HDR_SIZE;
        }
        b = next_blk(b);
    }
    spin_unlock_irqrestore(&heap_lock, flags);
    return NULL;
}

void *kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) { uint8_t *q = p; for (size_t i=0;i<size;i++) q[i]=0; }
    return p;
}

void kfree(void *ptr) {
    if (!ptr) return;
    uint32_t flags = spin_lock_irqsave(&heap_lock);
    blk_t *b = (blk_t *)((uint8_t *)ptr - HDR_SIZE);
    if (!in_heap(b)) { spin_unlock_irqrestore(&heap_lock, flags); return; }
    if (b->magic != MAGIC_USED) { spin_unlock_irqrestore(&heap_lock, flags); return; }
    b->magic = MAGIC_FREE;


    blk_t *cur = heap_start();
    while (in_heap(cur)) {
        blk_t *nx = next_blk(cur);
        if (cur->magic == MAGIC_FREE && in_heap(nx) && nx->magic == MAGIC_FREE) {
            cur->size += HDR_SIZE + nx->size;

        } else {
            cur = nx;
        }
    }
    spin_unlock_irqrestore(&heap_lock, flags);
}

void kmalloc_init(void *heap_start_arg, size_t heap_size) {
    (void)heap_start_arg; (void)heap_size;
    uint32_t flags = spin_lock_irqsave(&heap_lock);
    heap_init();
    spin_unlock_irqrestore(&heap_lock, flags);
}

size_t kmalloc_free_bytes(void) {
    uint32_t flags = spin_lock_irqsave(&heap_lock);
    if (!g_heap_ready) heap_init();
    size_t free = 0;
    blk_t *b = heap_start();
    while (in_heap(b)) {
        if (b->magic == MAGIC_FREE) free += b->size;
        b = next_blk(b);
    }
    spin_unlock_irqrestore(&heap_lock, flags);
    return free;
}




