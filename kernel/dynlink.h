








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


int module_run(const char *name);






