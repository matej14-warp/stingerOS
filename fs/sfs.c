/* scorpion OS - fs/sfs.c
   ScorpionFS - Simple in-memory filesystem
   Layout: flat node tree, no fragmentation, RAM-backed     */

#include "sfs.h"
#include "sfs_disk.h"
#include "../kernel/kmalloc.h"
#include "../kernel/terminal.h"
#include <stdint.h>
#include <stddef.h>

/* ---- string helpers ---- */
static int kstrcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void kstrcpy(char *d, const char *s) {
    while ((*d++ = *s++));
}
static void kmemcpy(void *d, const void *s, size_t n) {
    uint8_t *dd = d; const uint8_t *ss = s;
    while (n--) *dd++ = *ss++;
}

/* ---- node pool ---- */
#define MAX_NODES 512
static sfs_node_t node_pool[MAX_NODES];
static int        node_used[MAX_NODES];

static sfs_node_t *alloc_node(void) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (!node_used[i]) {
            node_used[i] = 1;
            /* zero the node */
            uint8_t *p = (uint8_t*)&node_pool[i];
            for (size_t j = 0; j < sizeof(sfs_node_t); j++) p[j] = 0;
            return &node_pool[i];
        }
    }
    return NULL;
}

static void free_node(sfs_node_t *n) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (&node_pool[i] == n) {
            node_used[i] = 0;
            return;
        }
    }
}

/* ---- root ---- */
static sfs_node_t *fs_root = NULL;

void sfs_init(void) {
    for (int i = 0; i < MAX_NODES; i++) node_used[i] = 0;
    fs_root = alloc_node();
    kstrcpy(fs_root->name, "/");
    fs_root->type   = SFS_DIR;
    fs_root->parent = NULL;
    fs_root->child  = NULL;
    fs_root->next   = NULL;
    fs_root->data   = NULL;
    fs_root->size   = 0;

    /* Create default directories */
    sfs_mkdir(fs_root, "bin");
    sfs_mkdir(fs_root, "etc");
    sfs_mkdir(fs_root, "modules");
    sfs_mkdir(fs_root, "tmp");
    sfs_mkdir(fs_root, "dev");

    /* /etc/version */
    sfs_node_t *etc = sfs_find_child(fs_root, "etc");
    if (etc) sfs_write_file(etc, "version", "scorpion 1.0\n", 13);
}

sfs_node_t *sfs_root(void) { return fs_root; }

/* Find direct child by name */
sfs_node_t *sfs_find_child(sfs_node_t *dir, const char *name) {
    if (!dir || dir->type != SFS_DIR) return NULL;
    sfs_node_t *cur = dir->child;
    while (cur) {
        if (kstrcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/* Add child to directory */
static void dir_add_child(sfs_node_t *dir, sfs_node_t *child) {
    child->parent = dir;
    child->next   = dir->child;
    dir->child    = child;
}

sfs_node_t *sfs_mkdir(sfs_node_t *parent, const char *name) {
    if (sfs_find_child(parent, name)) return NULL; /* already exists */
    sfs_node_t *n = alloc_node();
    if (!n) return NULL;
    kstrcpy(n->name, name);
    n->type = SFS_DIR;
    dir_add_child(parent, n);
    sfs_persist();
    return n;
}

sfs_node_t *sfs_create_file(sfs_node_t *parent, const char *name,
                              const void *data, size_t size) {
    if (sfs_find_child(parent, name)) return NULL;
    sfs_node_t *n = alloc_node();
    if (!n) return NULL;
    kstrcpy(n->name, name);
    n->type = SFS_FILE;
    if (size && data) {
        n->data = kmalloc(size);
        if (n->data) { kmemcpy(n->data, data, size); n->size = size; }
    }
    dir_add_child(parent, n);
    sfs_persist();
    return n;
}

/* Convenience: create/overwrite a file under a directory */
sfs_node_t *sfs_write_file(sfs_node_t *parent, const char *name,
                            const void *data, size_t size) {
    sfs_node_t *n = sfs_find_child(parent, name);
    if (!n) return sfs_create_file(parent, name, data, size);
    /* overwrite */
    if (n->data) { kfree(n->data); n->data = NULL; n->size = 0; }
    if (size && data) {
        n->data = kmalloc(size);
        if (n->data) { kmemcpy(n->data, data, size); n->size = size; }
    }
    sfs_persist();
    return n;
}

int sfs_delete(sfs_node_t *node) {
    if (!node || !node->parent) return -1; /* can't delete root */
    /* Remove from parent's child list */
    sfs_node_t *parent = node->parent;
    if (parent->child == node) {
        parent->child = node->next;
    } else {
        sfs_node_t *prev = parent->child;
        while (prev && prev->next != node) prev = prev->next;
        if (prev) prev->next = node->next;
    }
    if (node->data) kfree(node->data);
    free_node(node);
    sfs_persist();
    return 0;
}

/* Resolve a path string (absolute or relative to cwd) */
sfs_node_t *sfs_resolve(sfs_node_t *cwd, const char *path) {
    if (!path) return NULL;
    sfs_node_t *cur = (path[0] == '/') ? fs_root : cwd;
    if (path[0] == '/' && path[1] == '\0') return fs_root;

    /* Walk components */
    char buf[SFS_NAME_MAX];
    const char *p = path;
    if (*p == '/') p++;

    while (*p) {
        /* extract component */
        size_t i = 0;
        while (*p && *p != '/') buf[i++] = *p++;
        buf[i] = '\0';
        if (*p == '/') p++;

        if (i == 0 || (buf[0] == '.' && buf[1] == '\0')) continue;
        if (buf[0] == '.' && buf[1] == '.' && buf[2] == '\0') {
            if (cur->parent) cur = cur->parent;
            continue;
        }
        cur = sfs_find_child(cur, buf);
        if (!cur) return NULL;
    }
    return cur;
}

/* List directory entries (calls callback for each child) */
void sfs_list(sfs_node_t *dir, void (*cb)(sfs_node_t*, void*), void *userdata) {
    if (!dir || dir->type != SFS_DIR) return;
    sfs_node_t *cur = dir->child;
    while (cur) {
        cb(cur, userdata);
        cur = cur->next;
    }
}

/* Build an absolute path string into buf (max buflen bytes) */
void sfs_path(sfs_node_t *node, char *buf, size_t buflen) {
    if (!node || buflen == 0) return;
    if (node == fs_root) { buf[0] = '/'; buf[1] = '\0'; return; }

    /* Build reversed component list */
    const char *parts[64];
    int nparts = 0;
    sfs_node_t *cur = node;
    while (cur && cur != fs_root) {
        parts[nparts++] = cur->name;
        cur = cur->parent;
    }

    size_t pos = 0;
    for (int i = nparts-1; i >= 0 && pos < buflen-2; i--) {
        buf[pos++] = '/';
        for (const char *s = parts[i]; *s && pos < buflen-1; s++)
            buf[pos++] = *s;
    }
    buf[pos] = '\0';
}

/* ---- chmod / chown / du (v2 additions) ---- */
int sfs_chmod(sfs_node_t *node, uint16_t mode) {
    if (!node) return -1;
    node->mode = mode;
    return 0;
}

int sfs_chown(sfs_node_t *node, uint32_t uid, uint32_t gid) {
    if (!node) return -1;
    node->uid = uid; node->gid = gid;
    return 0;
}

/* Return the parent directory node for a given output path string.
   If path contains '/', resolves the directory portion; otherwise returns cwd. */
sfs_node_t *sfs_parent(sfs_node_t *cwd, const char *path) {
    if (!path) return cwd;
    /* Find last slash */
    int last = -1;
    for (int i = 0; path[i]; i++) if (path[i] == '/') last = i;
    if (last < 0) return cwd; /* no slash — parent is cwd */
    if (last == 0) return fs_root; /* e.g. "/foo" → parent is root */
    /* Extract directory portion */
    char buf[256];
    int i = 0;
    for (; i < last && i < 254; i++) buf[i] = path[i];
    buf[i] = '\0';
    sfs_node_t *p = sfs_resolve(cwd, buf);
    return p ? p : cwd;
}

void sfs_du(sfs_node_t *dir, uint32_t *nodes_out, uint32_t *bytes_out) {
    if (!dir) return;
    (*nodes_out)++;
    if (dir->type == SFS_FILE) { *bytes_out += (uint32_t)dir->size; return; }
    sfs_node_t *cur = dir->child;
    while (cur) { sfs_du(cur, nodes_out, bytes_out); cur = cur->next; }
}
