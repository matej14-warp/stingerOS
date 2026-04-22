



void *kmalloc(size_t size);
void *kzalloc(size_t size);
void  kfree(void *ptr);
void  kmalloc_init(void *heap_start, size_t heap_size);
size_t kmalloc_free_bytes(void);





