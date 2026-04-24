/* scorpion OS - kernel/kmalloc.h
   Simple heap allocator — bump allocator with free-list coalescing. */
#ifndef KMALLOC_H
#define KMALLOC_H
#include <stddef.h>
void *kmalloc(size_t size);
void *kzalloc(size_t size);   /* kmalloc + zero */
void  kfree(void *ptr);
void  kmalloc_init(void *heap_start, size_t heap_size);
size_t kmalloc_free_bytes(void);
#endif
