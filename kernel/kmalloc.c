/* scorpion OS - kernel/kmalloc.c
   Free-list heap allocator.
   Heap lives in BSS: 8 MB static arena placed above the kernel.
   Each block: [size:4][flags:4][payload...]
   flags bit 0 = 1 means free.  Adjacent free blocks are coalesced on free().
*/
#include "kmalloc.h"
#include <stddef.h>
#include <stdint.h>

#define HEAP_SIZE (8 * 1024 * 1024)   /* 8 MB */
#define ALIGN     8
#define HDR_SIZE  8    /* 4 bytes size + 4 bytes magic/flags */
#define MAGIC_FREE 0xF2EE1234u
#define MAGIC_USED 0xA11CA1EDu

static uint8_t  g_heap[HEAP_SIZE] __attribute__((aligned(16)));
static int      g_heap_ready = 0;

typedef struct blk {
    uint32_t size;   /* payload size (not including header) */
    uint32_t magic;  /* MAGIC_FREE or MAGIC_USED */
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
    if (!g_heap_ready) heap_init();

    /* Align up to ALIGN */
    size = (size + ALIGN - 1) & ~(size_t)(ALIGN - 1);
    if (size < ALIGN) size = ALIGN;

    blk_t *b = heap_start();
    while (in_heap(b)) {
        if (b->magic == MAGIC_FREE && b->size >= size) {
            /* Split if remainder large enough */
            if (b->size >= size + HDR_SIZE + ALIGN) {
                blk_t *rest = (blk_t *)((uint8_t *)b + HDR_SIZE + size);
                rest->size  = b->size - size - HDR_SIZE;
                rest->magic = MAGIC_FREE;
                b->size     = (uint32_t)size;
            }
            b->magic = MAGIC_USED;
            return (uint8_t *)b + HDR_SIZE;
        }
        b = next_blk(b);
    }
    return NULL;   /* out of memory */
}

void *kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) { uint8_t *q = p; for (size_t i=0;i<size;i++) q[i]=0; }
    return p;
}

void kfree(void *ptr) {
    if (!ptr) return;
    blk_t *b = (blk_t *)((uint8_t *)ptr - HDR_SIZE);
    if (!in_heap(b)) return;
    if (b->magic != MAGIC_USED) return;
    b->magic = MAGIC_FREE;

    /* Coalesce forward */
    blk_t *cur = heap_start();
    while (in_heap(cur)) {
        blk_t *nx = next_blk(cur);
        if (cur->magic == MAGIC_FREE && in_heap(nx) && nx->magic == MAGIC_FREE) {
            cur->size += HDR_SIZE + nx->size;
            /* Don't advance — recheck same block */
        } else {
            cur = nx;
        }
    }
}

void kmalloc_init(void *heap_start_arg, size_t heap_size) {
    (void)heap_start_arg; (void)heap_size;
    heap_init();
}

size_t kmalloc_free_bytes(void) {
    if (!g_heap_ready) heap_init();
    size_t free = 0;
    blk_t *b = heap_start();
    while (in_heap(b)) {
        if (b->magic == MAGIC_FREE) free += b->size;
        b = next_blk(b);
    }
    return free;
}
