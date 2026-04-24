/* scorpion OS - kernel/dynlink.c
   Minimal dynamic linker / module system.
   Modules are flat position-independent code blobs stored in ScorpionFS
   under /modules/<name>/<name>.bin.  They export a single entry symbol
   at offset 0 with signature  void module_main(void).
   Full ELF relocation is deferred; flat PIC blobs work with gcc -fpic -shared
   if the blob is loaded at the address it was linked at (identity mapped).  */

#include "dynlink.h"
#include "kmalloc.h"
#include "terminal.h"
#include "../fs/sfs.h"
#include <stdint.h>
#include <stddef.h>

dynmod_t g_modules[MAX_MODULES];
int      g_module_count = 0;

static void memset0(void *d,int v,int n){uint8_t*p=d;while(n--)*p++=(uint8_t)v;}
static void memcpy0(void *d,const void *s,int n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}
static int scmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return(unsigned char)*a-(unsigned char)*b;}
static void sncpy(char *d,const char *s,int n){int i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]=0;}

void dynlink_init(void) {
    memset0(g_modules, 0, sizeof(g_modules));
    g_module_count = 0;
}

int dynlink_load(const char *name, const uint8_t *data, size_t len) {
    if (g_module_count >= MAX_MODULES) {
        terminal_printf("dynlink: module table full\n");
        return -1;
    }
    /* Check if already loaded */
    for (int i=0;i<g_module_count;i++)
        if (g_modules[i].loaded && scmp(g_modules[i].name, name)==0)
            return 0;

    void *base = kmalloc(len);
    if (!base) {
        terminal_printf("dynlink: OOM loading '%s'\n", name);
        return -1;
    }
    memcpy0(base, data, (int)len);

    dynmod_t *m = &g_modules[g_module_count];
    sncpy(m->name, name, 64);
    m->base       = base;
    m->size       = len;
    m->n_exports  = 0;
    m->loaded     = 1;
    /* Export the entry point at offset 0 as "module_main" */
    sncpy(m->exports[0].name, "module_main", SYM_NAME_MAX);
    m->exports[0].addr = base;
    m->n_exports = 1;

    g_module_count++;
    terminal_printf("[dynlink] loaded '%s' at %p (%u bytes)\n",
                    name, base, (unsigned)len);
    return 0;
}

void *dynlink_sym(const char *modname, const char *symname) {
    for (int i=0;i<g_module_count;i++) {
        if (!g_modules[i].loaded) continue;
        if (modname && scmp(g_modules[i].name, modname)!=0) continue;
        for (int j=0;j<g_modules[i].n_exports;j++)
            if (scmp(g_modules[i].exports[j].name, symname)==0)
                return g_modules[i].exports[j].addr;
    }
    return NULL;
}

void dynlink_unload(const char *name) {
    for (int i=0;i<g_module_count;i++) {
        if (!g_modules[i].loaded || scmp(g_modules[i].name, name)!=0) continue;
        kfree(g_modules[i].base);
        memset0(&g_modules[i], 0, sizeof(dynmod_t));
        /* compact */
        for (int j=i;j<g_module_count-1;j++) g_modules[j]=g_modules[j+1];
        memset0(&g_modules[g_module_count-1], 0, sizeof(dynmod_t));
        g_module_count--;
        terminal_printf("[dynlink] unloaded '%s'\n", name);
        return;
    }
    terminal_printf("dynlink: module '%s' not found\n", name);
}

void dynlink_list(void) {
    if (!g_module_count) { terminal_printf("(no modules loaded)\n"); return; }
    terminal_printf("%-20s  %-10s  %s\n", "Name", "Size", "Entry");
    for (int i=0;i<g_module_count;i++) {
        if (!g_modules[i].loaded) continue;
        terminal_printf("%-20s  %-10u  %p\n",
            g_modules[i].name,
            (unsigned)g_modules[i].size,
            g_modules[i].base);
    }
}

int module_run(const char *name) {
    /* Check already-loaded modules first */
    void *entry = dynlink_sym(name, "module_main");
    if (entry) {
        void (*fn)(void) = (void (*)(void))entry;
        fn();
        return 0;
    }

    /* Try loading from /modules/<name>/<name>.bin in SFS */
    char path[128];
    int pi=0;
    const char *prefix = "/modules/";
    for (;prefix[pi];pi++) path[pi]=prefix[pi];
    for (int j=0;name[j]&&pi<120;j++) path[pi++]=name[j];
    path[pi]='/'; pi++;
    for (int j=0;name[j]&&pi<122;j++) path[pi++]=name[j];
    const char *ext=".bin";
    for (int j=0;ext[j]&&pi<126;j++) path[pi++]=ext[j];
    path[pi]=0;

    sfs_node_t *node = sfs_resolve(sfs_root(), path);
    if (!node || node->type != SFS_FILE || !node->data) {
        terminal_printf("module_run: '%s' not found in /modules/\n", name);
        return -1;
    }

    if (dynlink_load(name, node->data, node->size) != 0) return -1;

    entry = dynlink_sym(name, "module_main");
    if (!entry) {
        terminal_printf("module_run: no entry point in '%s'\n", name);
        return -1;
    }
    void (*fn)(void) = (void (*)(void))entry;
    fn();
    return 0;
}
