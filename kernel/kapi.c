







extern uint32_t get_ticks(void);
extern void     sleep_ms(uint32_t ms);

void kapi_init(void) {

}

void *kapi_kmalloc(size_t size)     { return kmalloc(size); }
void  kapi_kfree(void *ptr)         { kfree(ptr); }
void  kapi_print(const char *s)     { terminal_writestring(s); }
void  kapi_putchar(char c)          { terminal_putchar(c); }
int   kapi_getchar(void)            { return (int)keyboard_getchar(); }
void  kapi_sleep(uint32_t ms)       { sleep_ms(ms); }
uint32_t kapi_ticks(void)           { return get_ticks(); }




