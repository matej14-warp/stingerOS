/* scorpion OS - shell/nano.h */
#ifndef NANO_H
#define NANO_H
#include "../fs/sfs.h"
void nano_open(const char *filename, sfs_node_t *cwd);
#endif
