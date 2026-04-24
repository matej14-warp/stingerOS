#ifndef SFS_DISK_H
#define SFS_DISK_H

/* scorpion OS - fs/sfs_disk.h
   On-disk layout for ScorpionFS v2

   Partition layout (LBA relative to partition lba_start):
   -------------------------------------------------------
   LBA 0       : Superblock  (1 sector, 512 bytes)
   LBA 1..NT   : Node table  (SFS_DISK_MAX_NODES * sizeof(disk_node_t), rounded up)
   LBA NT+1..  : Data region (file contents, SFS_DISK_BLOCK_SIZE per block)

   A SYNC_TARGET partition has label "SYNC_TARGET" and type PART_TYPE_SFS (0xAF).
   sync  -> serialize RAM tree -> write to disk
   mount -> read disk -> rebuild RAM tree                                       */

#include <stdint.h>
#include <stddef.h>
#include "../fs/sfs.h"
#include "../fs/partition.h"

#define SFS_DISK_MAGIC       0x53465332UL  /* "SFS2" */
#define SFS_DISK_VERSION     1
#define SFS_DISK_MAX_NODES   512
#define SFS_DISK_NAME_MAX    64
#define SFS_DISK_BLOCK_SIZE  4096          /* bytes per data block = 8 sectors */
#define SFS_DISK_SECS_PER_BLOCK  (SFS_DISK_BLOCK_SIZE / 512)
#define SFS_DISK_NULL_IDX    0xFFFF
#define SYNC_TARGET_LABEL    "SYNC_TARGET"

/* ---- On-disk superblock (fits in 1 sector) ---- */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t node_count;        /* number of valid nodes stored */
    uint32_t node_table_lba;    /* relative LBA of node table start (always 1) */
    uint32_t node_table_secs;   /* number of sectors occupied by node table */
    uint32_t data_region_lba;   /* relative LBA where data blocks start */
    uint32_t data_block_count;  /* total data blocks written */
    uint8_t  reserved[512 - 28];
} __attribute__((packed)) sfs_superblock_t;

/* ---- On-disk node (one entry in the node table) ---- */
typedef struct {
    char     name[SFS_DISK_NAME_MAX];   /* 64 bytes */
    uint8_t  type;                       /*  1 byte  - SFS_FILE or SFS_DIR */
    uint8_t  valid;                      /*  1 byte  - 1 = slot in use */
    uint16_t parent_idx;                 /*  2 bytes - SFS_DISK_NULL_IDX for root */
    uint32_t size;                       /*  4 bytes - file data size in bytes */
    uint32_t data_block;                 /*  4 bytes - index into data region */
    uint8_t  reserved[52];              /* 52 bytes - pad to 128 bytes total */
} __attribute__((packed)) sfs_disk_node_t;

/* sizeof(sfs_disk_node_t) must divide cleanly - we enforce 128 bytes */
#define SFS_DISK_NODE_SIZE   128
#define SFS_DISK_NODES_PER_SECTOR (512 / SFS_DISK_NODE_SIZE)   /* 4 */
#define SFS_DISK_NODE_TABLE_SECS  (SFS_DISK_MAX_NODES / SFS_DISK_NODES_PER_SECTOR)  /* 128 */

/* Public API */
int  sfs_sync_to_disk(int drive, uint32_t part_lba_start);
int  sfs_load_from_disk(int drive, uint32_t part_lba_start);
int  sfs_find_sync_target(const part_entry_t *parts, int count,
                           int *drive_out, uint32_t *lba_out);

/* Shell-facing helpers */
int  cmd_sync(const part_entry_t *parts, int count);
int  cmd_mount_disk(const part_entry_t *parts, int count);

#endif /* SFS_DISK_H */
