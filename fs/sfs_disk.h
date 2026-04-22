



















typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t node_count;
    uint32_t node_table_lba;
    uint32_t node_table_secs;
    uint32_t data_region_lba;
    uint32_t data_block_count;
    uint8_t  reserved[512 - 28];
} __attribute__((packed)) sfs_superblock_t;


typedef struct {
    char     name[SFS_DISK_NAME_MAX];
    uint8_t  type;
    uint8_t  valid;
    uint16_t parent_idx;
    uint32_t size;
    uint32_t data_block;
    uint8_t  reserved[52];
} __attribute__((packed)) sfs_disk_node_t;







int  sfs_sync_to_disk(int drive, uint32_t part_lba_start);
int  sfs_load_from_disk(int drive, uint32_t part_lba_start);
int  sfs_find_sync_target(const part_entry_t *parts, int count,
                           int *drive_out, uint32_t *lba_out);


int  cmd_sync(const part_entry_t *parts, int count);
int  cmd_mount_disk(const part_entry_t *parts, int count);


void sfs_set_persist_target(int drive, uint32_t lba);
void sfs_persist(void);
void sfs_persist_flush(void);
void sfs_persist_suppress(int s);






