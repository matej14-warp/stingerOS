/* scorpion OS - kernel/kmalloc.c
   Simple bump allocator + free list for kernel heap           */

#include "kmalloc.h"
#include <stdint.h>
#include <stddef.h>

/* Kernel heap starts at 4 MiB, 4 MiB in size */
#define HEAP_START  0x400000
#define HEAP_SIZE   0x400000

typedef struct block_header {
    size_t size;
    int    free;
    struct block_header *next;
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

static block_header_t *heap_head = NULL;
static int heap_inited = 0;

static void heap_init(void) {
    heap_head = (block_header_t*)HEAP_START;
    heap_head->size = HEAP_SIZE - HEADER_SIZE;
    heap_head->free = 1;
    heap_head->next = NULL;
    heap_inited = 1;
}

static void split(block_header_t *b, size_t size) {
    if (b->size < size + HEADER_SIZE + 16) return; /* too small to split */
    block_header_t *newb = (block_header_t*)((uint8_t*)b + HEADER_SIZE + size);
    newb->size = b->size - size - HEADER_SIZE;
    newb->free = 1;
    newb->next = b->next;
    b->size    = size;
    b->next    = newb;
}

void *kmalloc(size_t size) {
    if (!heap_inited) heap_init();
    if (size == 0) return NULL;
    /* Align to 4 bytes */
    size = (size + 3) & ~3;

    block_header_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            split(cur, size);
            cur->free = 0;
            return (void*)((uint8_t*)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }
    return NULL; /* OOM */
}

void *kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) {
        uint8_t *b = (uint8_t*)p;
        for (size_t i = 0; i < size; i++) b[i] = 0;
    }
    return p;
}

static void merge(void) {
    block_header_t *cur = heap_head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += HEADER_SIZE + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_header_t *b = (block_header_t*)((uint8_t*)ptr - HEADER_SIZE);
    b->free = 1;
    merge();
}

size_t kheap_free(void) {
    if (!heap_inited) return HEAP_SIZE;
    size_t total = 0;
    block_header_t *cur = heap_head;
    while (cur) {
        if (cur->free) total += cur->size;
        cur = cur->next;
    }
    return total;
}
