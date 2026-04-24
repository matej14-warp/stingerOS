/* scorpion OS - kernel/dynlink.h
   Dynamic module loader: loads ELF32 .so or flat binary modules into
   kmalloc'd memory and resolves a simple symbol table.               */
#ifndef DYNLINK_H
#define DYNLINK_H
#include <stddef.h>
#include <stdint.h>

#define MAX_MODULES  16
#define SYM_NAME_MAX 48

typedef struct {
    char     name[SYM_NAME_MAX];
    void    *addr;
} dynsym_t;

typedef struct {
    char     name[64];
    void    *base;
    size_t   size;
    dynsym_t exports[16];
    int      n_exports;
    int      loaded;
} dynmod_t;

extern dynmod_t g_modules[MAX_MODULES];
extern int      g_module_count;

void  dynlink_init(void);
int   dynlink_load(const char *name, const uint8_t *data, size_t len);
void *dynlink_sym(const char *modname, const char *symname);
void  dynlink_unload(const char *name);
void  dynlink_list(void);

/* Run a raw flat binary as a module (for .mbf / user-compiled code) */
int module_run(const char *name);

#endif /* DYNLINK_H */
