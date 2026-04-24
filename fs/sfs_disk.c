/* scorpion OS - fs/sfs_disk.c
   ScorpionFS on-disk persistence layer

   sync  : walks the in-memory node tree, assigns each node an index,
           writes the node table and data blocks to the ATA partition,
           then writes the superblock.

   load  : reads superblock, validates magic, reads node table,
           reconstructs the in-memory tree by creating nodes in
           index order (parents always have lower indices than children
           because we do a BFS-order write during sync).              */

#include "sfs_disk.h"
#include "sfs.h"
#include "partition.h"
#include "../drivers/ata.h"
#include "../kernel/terminal.h"
#include "../kernel/kmalloc.h"
#include <stdint.h>
#include <stddef.h>

/* ---- helpers ---- */
static void k_memset(void *d, uint8_t v, size_t n){uint8_t*p=d;while(n--)*p++=v;}
static void k_memcpy(void *d,const void*s,size_t n){uint8_t*dd=d;const uint8_t*ss=s;while(n--)*dd++=*ss++;}
static int  k_strcmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return(unsigned char)*a-(unsigned char)*b;}
static void k_strcpy(char*d,const char*s){while((*d++=*s++));}
static size_t k_strlen(const char*s){size_t n=0;while(s[n])n++;return n;}

/* ---- sector-aligned scratch buffers (static = in BSS, not stack) ---- */
static uint8_t  g_sector_buf[512];
static uint8_t  g_node_sector[512];   /* one sector of node table */
static uint8_t  g_data_block[SFS_DISK_BLOCK_SIZE]; /* one data block */

/* ---- node index assignment (BFS order, root = 0) ---- */
#define QUEUE_SIZE SFS_DISK_MAX_NODES
static sfs_node_t *bfs_queue[QUEUE_SIZE];
static uint16_t    bfs_parent_idx[QUEUE_SIZE];  /* parent index for each queued node */

/* Returns number of nodes enumerated, fills bfs_queue[] and bfs_parent_idx[] */
static int enumerate_nodes(void){
    int head=0, tail=0, count=0;
    sfs_node_t *root=sfs_root();
    if(!root) return 0;

    bfs_queue[tail]=root;
    bfs_parent_idx[tail]=SFS_DISK_NULL_IDX;
    tail++;
    count++;

    while(head<tail && count<SFS_DISK_MAX_NODES){
        sfs_node_t *cur=bfs_queue[head];
        uint16_t cur_idx=(uint16_t)head;
        head++;

        if(cur->type==SFS_DIR){
            sfs_node_t *child=cur->child;
            while(child && count<SFS_DISK_MAX_NODES){
                bfs_queue[tail]=child;
                bfs_parent_idx[tail]=cur_idx;
                tail++;
                count++;
                child=child->next;
            }
        }
    }
    return count;
}

/* ============================================================
   sfs_sync_to_disk
   ============================================================ */
int sfs_sync_to_disk(int drive, uint32_t part_lba){
    terminal_printf("[sync] starting sync to drive %d lba %u\n", drive, part_lba);

    /* ---- enumerate nodes in BFS order ---- */
    int node_count = enumerate_nodes();
    if(node_count==0){
        terminal_printf("[sync] filesystem is empty, nothing to sync\n");
        return 0;
    }
    terminal_printf("[sync] %d nodes to write\n", node_count);

    /* ---- write node table (SFS_DISK_NODE_TABLE_SECS sectors after LBA 0) ---- */
    /* LBA layout: 0=superblock, 1..128=node table, 129+=data blocks */
    const uint32_t node_table_lba = 1;
    const uint32_t data_region_lba = node_table_lba + SFS_DISK_NODE_TABLE_SECS;

    /* We write node table 4 nodes (1 sector) at a time */
    uint32_t data_block_idx = 0;  /* running counter for data blocks */

    for(int sector_i=0; sector_i < SFS_DISK_NODE_TABLE_SECS; sector_i++){
        k_memset(g_node_sector, 0, 512);
        for(int slot=0; slot<SFS_DISK_NODES_PER_SECTOR; slot++){
            int node_idx = sector_i * SFS_DISK_NODES_PER_SECTOR + slot;
            sfs_disk_node_t *dn = (sfs_disk_node_t*)(g_node_sector + slot*SFS_DISK_NODE_SIZE);

            if(node_idx >= node_count){
                dn->valid=0;
                continue;
            }

            sfs_node_t *n = bfs_queue[node_idx];
            k_memset(dn, 0, SFS_DISK_NODE_SIZE);
            k_strcpy(dn->name, n->name);
            dn->type       = (uint8_t)n->type;
            dn->valid      = 1;
            dn->parent_idx = bfs_parent_idx[node_idx];
            dn->size       = (uint32_t)n->size;
            dn->data_block = (uint32_t)SFS_DISK_NULL_IDX;  /* filled below if file */

            if(n->type==SFS_FILE && n->data && n->size>0){
                /* Assign next data block */
                dn->data_block = data_block_idx;
                data_block_idx++;
            }
        }
        uint32_t abs_lba = part_lba + node_table_lba + (uint32_t)sector_i;
        if(ata_write_sectors(drive, abs_lba, 1, g_node_sector)!=0){
            terminal_printf("[sync] error: node table write failed at sector %d\n", sector_i);
            return -1;
        }
    }

    /* ---- write data blocks ---- */
    uint32_t db=0;
    for(int i=0; i<node_count; i++){
        sfs_node_t *n=bfs_queue[i];
        if(n->type!=SFS_FILE || !n->data || n->size==0) continue;

        k_memset(g_data_block, 0, SFS_DISK_BLOCK_SIZE);
        size_t copy_len = n->size < SFS_DISK_BLOCK_SIZE ? n->size : SFS_DISK_BLOCK_SIZE;
        k_memcpy(g_data_block, n->data, copy_len);

        /* Write block as SFS_DISK_SECS_PER_BLOCK consecutive sectors */
        for(int s=0; s<SFS_DISK_SECS_PER_BLOCK; s++){
            uint32_t abs_lba = part_lba + data_region_lba
                             + db * SFS_DISK_SECS_PER_BLOCK + (uint32_t)s;
            if(ata_write_sectors(drive, abs_lba, 1, g_data_block + s*512)!=0){
                terminal_printf("[sync] error: data write failed (block %u)\n", db);
                return -1;
            }
        }
        db++;
    }

    /* ---- write superblock last ---- */
    k_memset(g_sector_buf, 0, 512);
    sfs_superblock_t *sb = (sfs_superblock_t*)g_sector_buf;
    sb->magic            = SFS_DISK_MAGIC;
    sb->version          = SFS_DISK_VERSION;
    sb->node_count       = (uint32_t)node_count;
    sb->node_table_lba   = node_table_lba;
    sb->node_table_secs  = SFS_DISK_NODE_TABLE_SECS;
    sb->data_region_lba  = data_region_lba;
    sb->data_block_count = db;

    if(ata_write_sectors(drive, part_lba, 1, g_sector_buf)!=0){
        terminal_printf("[sync] error: superblock write failed\n");
        return -1;
    }

    terminal_printf("[sync] wrote %d nodes, %u data blocks (%u KB)\n",
        node_count, db, db * (SFS_DISK_BLOCK_SIZE/1024));
    terminal_printf("[sync] sync complete\n");
    return 0;
}

/* ============================================================
   sfs_load_from_disk
   ============================================================ */
int sfs_load_from_disk(int drive, uint32_t part_lba){
    terminal_printf("[mount] loading from drive %d lba %u\n", drive, part_lba);

    /* ---- read and validate superblock ---- */
    if(ata_read_sectors(drive, part_lba, 1, g_sector_buf)!=0){
        terminal_printf("[mount] error: cannot read superblock\n");
        return -1;
    }
    sfs_superblock_t *sb = (sfs_superblock_t*)g_sector_buf;
    if(sb->magic != SFS_DISK_MAGIC){
        terminal_printf("[mount] error: bad magic (got 0x%x, expected 0x%x)\n",
            sb->magic, SFS_DISK_MAGIC);
        return -1;
    }
    if(sb->version != SFS_DISK_VERSION){
        terminal_printf("[mount] error: unsupported version %u\n", sb->version);
        return -1;
    }

    int node_count = (int)sb->node_count;
    uint32_t node_table_lba  = sb->node_table_lba;
    uint32_t data_region_lba = sb->data_region_lba;

    terminal_printf("[mount] found %d nodes, %u data blocks\n",
        node_count, sb->data_block_count);

    if(node_count==0){
        terminal_printf("[mount] empty filesystem, nothing to load\n");
        return 0;
    }

    /* ---- allocate flat node-pointer array for index lookup ---- */
    sfs_node_t **node_map = (sfs_node_t**)kmalloc(
        (size_t)node_count * sizeof(sfs_node_t*));
    if(!node_map){
        terminal_printf("[mount] OOM: cannot allocate node map\n");
        return -1;
    }
    k_memset(node_map, 0, (size_t)node_count * sizeof(sfs_node_t*));

    /* ---- read node table and create in-memory nodes ---- */
    /* Allocate a buffer for one data block (reused per file) */
    uint8_t *data_buf = (uint8_t*)kmalloc(SFS_DISK_BLOCK_SIZE);
    if(!data_buf){
        kfree(node_map);
        terminal_printf("[mount] OOM: cannot allocate data buffer\n");
        return -1;
    }

    /* Re-init SFS so we get a clean root — suppress persist during bulk load */
    sfs_persist_suppress(1);
    sfs_init();
    /* sfs_init() creates root + default dirs; we'll overwrite from disk */

    for(int sector_i=0; sector_i < SFS_DISK_NODE_TABLE_SECS; sector_i++){
        uint32_t abs_lba = part_lba + node_table_lba + (uint32_t)sector_i;
        if(ata_read_sectors(drive, abs_lba, 1, g_node_sector)!=0){
            terminal_printf("[mount] error: node table read failed at sector %d\n", sector_i);
            kfree(data_buf); kfree(node_map);
            return -1;
        }

        for(int slot=0; slot<SFS_DISK_NODES_PER_SECTOR; slot++){
            int node_idx = sector_i * SFS_DISK_NODES_PER_SECTOR + slot;
            if(node_idx >= node_count) goto done_nodes;

            sfs_disk_node_t *dn = (sfs_disk_node_t*)(g_node_sector + slot*SFS_DISK_NODE_SIZE);
            if(!dn->valid){ node_map[node_idx]=NULL; continue; }

            if(node_idx==0){
                /* Root node — reuse the existing root */
                node_map[0] = sfs_root();
            } else {
                /* Find parent */
                sfs_node_t *parent = NULL;
                if(dn->parent_idx < (uint16_t)node_count)
                    parent = node_map[dn->parent_idx];
                if(!parent) parent = sfs_root();

                if(dn->type==SFS_DIR){
                    /* Create or find existing dir */
                    sfs_node_t *existing = sfs_find_child(parent, dn->name);
                    if(existing && existing->type==SFS_DIR){
                        node_map[node_idx] = existing;
                    } else {
                        node_map[node_idx] = sfs_mkdir(parent, dn->name);
                    }
                } else {
                    /* File: read its data block */
                    void *file_data = NULL;
                    size_t file_size = (size_t)dn->size;

                    if(dn->data_block != (uint32_t)SFS_DISK_NULL_IDX && file_size>0){
                        k_memset(data_buf, 0, SFS_DISK_BLOCK_SIZE);
                        for(int s=0; s<SFS_DISK_SECS_PER_BLOCK; s++){
                            uint32_t dlba = part_lba + data_region_lba
                                          + dn->data_block * SFS_DISK_SECS_PER_BLOCK
                                          + (uint32_t)s;
                            if(ata_read_sectors(drive, dlba, 1, data_buf+s*512)!=0){
                                terminal_printf("[mount] warn: data read failed for '%s'\n",
                                    dn->name);
                                file_size=0; break;
                            }
                        }
                        if(file_size>0) file_data=data_buf;
                    }
                    node_map[node_idx] = sfs_write_file(parent, dn->name,
                                                         file_data, file_size);
                }
            }
        }
    }
done_nodes:

    kfree(data_buf);
    kfree(node_map);
    sfs_persist_suppress(0); /* re-enable auto-persist after bulk load */
    terminal_printf("[mount] filesystem loaded successfully\n");
    return 0;
}

/* ============================================================
   Active persistence target — set at boot by kernel
   ============================================================ */
static int      g_persist_drive = -1;
static uint32_t g_persist_lba   = 0;
static int      g_persist_ready = 0;

void sfs_set_persist_target(int drive, uint32_t lba) {
    g_persist_drive = drive;
    g_persist_lba   = lba;
    g_persist_ready = 1;
}

static int g_persist_suppress = 0;
void sfs_persist_suppress(int s) { g_persist_suppress = s; }

/* Debounce: only actually write if 500ms has passed since last mutation.
   This prevents nano saves and bulk ops from hammering the disk. */
static uint32_t g_persist_pending_tick = 0;
static int      g_persist_pending = 0;

/* Called after every write/mkdir/delete to flush to disk.
   Silently skips if no partition has been registered yet,
   or if suppressed (e.g. during bulk load). Uses 500ms debounce. */
void sfs_persist(void) {
    if (!g_persist_ready || g_persist_suppress) return;
    extern uint32_t get_ticks(void);
    g_persist_pending_tick = get_ticks() + 500;
    g_persist_pending = 1;
}

/* Call from main kernel loop / idle to flush pending writes */
void sfs_persist_flush(void) {
    if (!g_persist_pending) return;
    extern uint32_t get_ticks(void);
    if ((uint32_t)(get_ticks() - g_persist_pending_tick) > 0x80000000u) return; /* not yet */
    g_persist_pending = 0;
    sfs_sync_to_disk(g_persist_drive, g_persist_lba);
}

/* ============================================================
   Find the SYNC_TARGET partition
   ============================================================ */
int sfs_find_sync_target(const part_entry_t *parts, int count,
                          int *drive_out, uint32_t *lba_out){
    for(int i=0;i<count;i++){
        if(parts[i].type==PART_TYPE_SFS &&
           k_strcmp(parts[i].label, SYNC_TARGET_LABEL)==0){
            *drive_out = parts[i].drive;
            *lba_out   = parts[i].lba_start;
            return i;
        }
    }
    return -1;
}

/* ============================================================
   Shell-facing wrappers
   ============================================================ */
int cmd_sync(const part_entry_t *parts, int count){
    int drive; uint32_t lba;
    int pi = sfs_find_sync_target(parts, count, &drive, &lba);
    if(pi<0){
        terminal_printf("sync: no SYNC_TARGET partition found\n");
        terminal_printf("  label a ScorpionFS partition with: setlabel <drive> <idx> SYNC_TARGET\n");
        return -1;
    }
    terminal_printf("sync: target /dev/hd%c%d\n", 'a'+parts[pi].drive, parts[pi].index);
    return sfs_sync_to_disk(drive, lba);
}

int cmd_mount_disk(const part_entry_t *parts, int count){
    int drive; uint32_t lba;
    int pi = sfs_find_sync_target(parts, count, &drive, &lba);
    if(pi<0){
        terminal_printf("mount: no SYNC_TARGET partition found\n");
        return -1;
    }
    terminal_printf("mount: loading from /dev/hd%c%d\n", 'a'+parts[pi].drive, parts[pi].index);
    return sfs_load_from_disk(drive, lba);
}
