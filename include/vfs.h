#ifndef _VFS_H
#define _VFS_H

#include <include/types.h>

struct vnode {
    struct mount *mount;
    struct vnode_operations *v_ops;
    struct file_operations *f_ops;
    void *internal;
};

struct file {
    struct vnode *vnode;
    size_t f_pos;
    struct file_operations *f_ops;
    int flags;
};

struct mount {
    struct vnode *root;
    struct filesystem *fs;
};

struct filesystem {
    const char *name;
    int (*setup_mount)(struct filesystem *fs, struct mount *mount);
};

struct file_operations {
    int (*write)(struct file *file, const void *buf, size_t len);
    int (*read)(struct file *file, void *buf, size_t len);
};

struct vnode_operations {
    int (*lookup)(struct vnode *dir_node,
                  struct vnode **target,
                  const char *component_name);
    int (*create)(struct vnode *dir_node,
                  struct vnode **target,
                  const char *component_name);
};

int register_filesystem(struct filesystem *fs);
struct file *vfs_open(const char *pathname, int flags);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_read(struct file *file, void *buf, size_t len);

#endif