/* scorpion OS - kernel/kapi.h
   Kernel API: syscall-like interface for user modules / MBF scripts.  */
#ifndef KAPI_H
#define KAPI_H
#include <stdint.h>
#include <stddef.h>

void kapi_init(void);

/* Exposed kernel services (callable from modules) */
void *kapi_kmalloc(size_t size);
void  kapi_kfree(void *ptr);
void  kapi_print(const char *s);
void  kapi_putchar(char c);
int   kapi_getchar(void);
void  kapi_sleep(uint32_t ms);
uint32_t kapi_ticks(void);

#endif /* KAPI_H */
