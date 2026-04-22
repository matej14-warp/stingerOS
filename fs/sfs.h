









typedef struct sfs_node {
    char            name[SFS_NAME_MAX];
    int             type;
    void           *data;
    size_t          size;
    struct sfs_node *parent;
    struct sfs_node *child;
    struct sfs_node *next;
    uint16_t        mode;
    uint32_t        uid;
    uint32_t        gid;
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

void        sfs_du(sfs_node_t *dir, uint32_t *nodes_out, uint32_t *bytes_out);

sfs_node_t *sfs_parent(sfs_node_t *cwd, const char *path);






