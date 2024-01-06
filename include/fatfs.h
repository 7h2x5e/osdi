#ifndef _FATFS_H
#define _FATFS_H

#include <include/types.h>
#include <include/vfs.h>

#define FATFS_FIEL_BUFFER_MAX_LEN 64

typedef struct fatfs_node {
    struct inode inode;
    uint32_t cluster;
} fatfs_node_t;

void fatfs_init();
struct dentry *fatfs_mount(struct filesystem *fs, const char *name, void *data);

#endif