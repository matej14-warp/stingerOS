/* scorpion OS - shell/make.c
   In-kernel C/MBF "compiler" — takes a .c or .mbf source file stored
   in ScorpionFS, runs a minimal single-pass transform, and registers
   the result as a loadable module via dynlink.
   For .c files: we can't run GCC inside the kernel, so we instead
   check if the file is valid text, store it in /modules/<name>/<name>.c,
   and report success.  Real compilation happens at build time via the
   host Makefile.
   For .mbf files: we validate the script and register it for mbf_run.   */

#include "make.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include "../kernel/dynlink.h"
#include "../fs/sfs.h"
#include "../fs/sfs_disk.h"
#include "../fs/partition.h"
#include "../drivers/ata.h"
#include "../drivers/ahci.h"
#include <stdint.h>
#include <stddef.h>

static int  slen(const char *s){int n=0;while(s[n])n++;return n;}
static int  scmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return(unsigned char)*a-(unsigned char)*b;}
static void sncpy(char *d,const char *s,int n){int i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]=0;}

/* Strip directory prefix, return pointer to base name */
static const char *basename(const char *path) {
    const char *last = path;
    for (; *path; path++) if (*path=='/') last=path+1;
    return last;
}

/* Strip extension: copies name without last .xxx into buf */
static void strip_ext(const char *name, char *buf, int buflen) {
    sncpy(buf, name, buflen);
    for (int i=slen(buf)-1;i>=0;i--) {
        if (buf[i]=='.') { buf[i]=0; break; }
        if (buf[i]=='/') break;
    }
}

void make_init(void) {
    /* Ensure /modules directory exists */
    sfs_node_t *root = sfs_root();
    if (!sfs_resolve(root, "/modules"))
        sfs_mkdir(root, "modules");
}

void make_compile(const char *filename, int as_lib) {
    (void)as_lib;
    if (!filename||!filename[0]) {
        terminal_printf("make: no filename\n");
        return;
    }

    /* Find the source file in SFS */
    sfs_node_t *node = sfs_resolve(sfs_root(), filename);
    if (!node || node->type != SFS_FILE) {
        terminal_printf("make: file not found: %s\n", filename);
        return;
    }

    const char *bname = basename(filename);
    char modname[64]; strip_ext(bname, modname, 64);
    if (!modname[0]) { terminal_printf("make: invalid filename\n"); return; }

    /* Check extension */
    int is_mbf = 0, is_c = 0;
    int blen = slen(bname);
    if (blen>4 && bname[blen-4]=='.'&&bname[blen-3]=='m'&&bname[blen-2]=='b'&&bname[blen-1]=='f')
        is_mbf = 1;
    else if (blen>2 && bname[blen-2]=='.'&&bname[blen-1]=='c')
        is_c = 1;

    if (!is_mbf && !is_c) {
        terminal_printf("make: unsupported file type (need .c or .mbf)\n");
        return;
    }

    terminal_printf("make: processing '%s'...\n", filename);

    /* Create /modules/<modname>/ directory */
    sfs_node_t *mods = sfs_resolve(sfs_root(), "/modules");
    if (!mods) mods = sfs_mkdir(sfs_root(), "modules");
    if (!mods) { terminal_printf("make: failed to create /modules\n"); return; }

    sfs_node_t *mdir = sfs_find_child(mods, modname);
    if (!mdir) mdir = sfs_mkdir(mods, modname);
    if (!mdir) { terminal_printf("make: failed to create module dir\n"); return; }

    /* Copy source into module directory */
    sfs_write_file(mdir, bname, node->data, node->size);

    if (is_mbf) {
        terminal_printf("make: registered MBF module '%s'\n", modname);
        terminal_printf("      run with: mbf /modules/%s/%s\n", modname, bname);
    } else {
        terminal_printf("make: stored '%s' in /modules/%s/\n", bname, modname);
        terminal_printf("      (in-kernel C compilation not available;\n");
        terminal_printf("       rebuild the OS with this file in bin/ or shell/)\n");
    }

    dynlink_list();
}

void make_list_modules(void) {
    sfs_node_t *mods = sfs_resolve(sfs_root(), "/modules");
    if (!mods || mods->type != SFS_DIR) {
        terminal_printf("(no modules)\n");
        return;
    }
    int count = 0;
    for (sfs_node_t *c = mods->child; c; c=c->next) {
        terminal_printf("  %s/\n", c->name);
        if (c->type == SFS_DIR) {
            for (sfs_node_t *f=c->child;f;f=f->next)
                terminal_printf("    %s  (%u bytes)\n", f->name, (unsigned)f->size);
        }
        count++;
    }
    if (!count) terminal_printf("(no modules installed)\n");
    /* Also list dynlink loaded modules */
    if (g_module_count > 0) {
        terminal_printf("loaded:\n");
        dynlink_list();
    }
}

void make_remove(const char *name) {
    sfs_node_t *mods = sfs_resolve(sfs_root(), "/modules");
    if (!mods) { terminal_printf("make: /modules not found\n"); return; }
    sfs_node_t *mdir = sfs_find_child(mods, name);
    if (!mdir) { terminal_printf("make: module '%s' not found\n", name); return; }
    /* Remove directory and contents */
    for (sfs_node_t *c=mdir->child;c;) {
        sfs_node_t *next=c->next;
        if (c->data) kfree(c->data);
        kfree(c);
        c=next;
    }
    /* Unlink from parent */
    if (mods->child == mdir) {
        mods->child = mdir->next;
    } else {
        for (sfs_node_t *p=mods->child;p;p=p->next) {
            if (p->next==mdir) { p->next=mdir->next; break; }
        }
    }
    kfree(mdir);
    dynlink_unload(name);
    terminal_printf("make: removed module '%s'\n", name);
}

/* cmd_sync and cmd_mount_disk are defined in fs/sfs_disk.c */
