#ifndef _TMPFS_H
#define _TMPFS_H

#include <include/types.h>
#include <include/vfs.h>

#define TMPFS_FIEL_BUFFER_MAX_LEN 64

typedef struct tmpfs_node {
    struct inode inode;
    uint8_t buffer[TMPFS_FIEL_BUFFER_MAX_LEN];
} tmpfs_node_t;

void tmpfs_init();
struct dentry *tmpfs_mount(struct filesystem *fs, const char *name, void *data);

#endif