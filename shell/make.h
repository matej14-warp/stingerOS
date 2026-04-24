/* scorpion OS - shell/make.h */
#ifndef MAKE_H
#define MAKE_H
#include "../fs/sfs.h"
#include "../fs/partition.h"
void make_init(void);
void make_compile(const char *filename, int as_lib);
void make_list_modules(void);
void make_remove(const char *name);
/* Disk sync helpers called from shell builtins */
/* signatures match sfs_disk.h */
int cmd_sync(const part_entry_t *parts, int count);
int cmd_mount_disk(const part_entry_t *parts, int count);
#endif
