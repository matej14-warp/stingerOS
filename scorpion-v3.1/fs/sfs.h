#ifndef SFS_H
#define SFS_H

#include <stddef.h>
#include <stdint.h>

#define SFS_NAME_MAX 64
#define SFS_FILE 0
#define SFS_DIR  1

typedef struct sfs_node {
    char            name[SFS_NAME_MAX];
    int             type;       /* SFS_FILE or SFS_DIR */
    void           *data;       /* file content (NULL for dirs) */
    size_t          size;
    struct sfs_node *parent;
    struct sfs_node *child;     /* first child (dir only) */
    struct sfs_node *next;      /* next sibling */
    uint16_t        mode;       /* Unix-style permission bits e.g. 0755 */
    uint32_t        uid;        /* owner user ID */
    uint32_t        gid;        /* owner group ID */
} sfs_node_t;

void        sfs_init(void);
sfs_node_t *sfs_root(void);
sfs_node_t *sfs_find_child(sfs_node_t *dir, const char *name);
sfs_node_t *sfs_mkdir(sfs_node_t *parent, const char *name);
sfs_node_t *sfs_create_file(sfs_node_t *parent, const char *name,
                              const void *data, size_t size);
sfs_node_t *sfs_write_file(sfs_node_t *parent, const char *name,
                            const void *data, size_t size);
int         sfs_delete(sfs_node_t *node);
sfs_node_t *sfs_resolve(sfs_node_t *cwd, const char *path);
void        sfs_list(sfs_node_t *dir, void (*cb)(sfs_node_t*, void*), void *userdata);
void        sfs_path(sfs_node_t *node, char *buf, size_t buflen);
int         sfs_chmod(sfs_node_t *node, uint16_t mode);
int         sfs_chown(sfs_node_t *node, uint32_t uid, uint32_t gid);
/* Count nodes and total bytes used under a dir (recursive) */
void        sfs_du(sfs_node_t *dir, uint32_t *nodes_out, uint32_t *bytes_out);

#endif
