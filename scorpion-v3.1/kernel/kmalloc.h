#ifndef KMALLOC_H
#define KMALLOC_H
#include <stddef.h>
void  *kmalloc(size_t size);
void  *kzalloc(size_t size);
void   kfree(void *ptr);
size_t kheap_free(void);
#endif
