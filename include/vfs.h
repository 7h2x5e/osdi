#ifndef _VFS_H
#define _VFS_H

#include <include/types.h>

#define MAX_CHILD_DIR 10

enum file_op_flag {
    O_CREAT = 0b1,
};

enum node_attr_flag {
    DIRECTORY,
    FILE,
};

struct vnode {
    struct mount *mount;
    struct vnode_operations *v_ops;
    struct file_operations *f_ops;
};

typedef struct dentry {
    char *name;
    enum node_attr_flag flag;
    struct vnode *vnode;
    int child_amount;
    struct dentry *parent;
    struct dentry *child[MAX_CHILD_DIR];
    void *internal;
} dentry_t;

typedef struct file {
    dentry_t *dentry;
    size_t f_pos;
} file_t;

typedef struct dir {
    off_t idx; /* Position of next directory entry to read.  */
    dentry_t *dentry;
} dir_t;

typedef struct dirent {
    dentry_t *dentry;
} dirent_t;

struct mount {
    struct filesystem *fs;
    struct dentry *root_dir;
};

struct filesystem {
    const char *name;
    int (*setup_mount)(struct filesystem *fs, struct mount *mount);
};

struct file_operations {
    int (*write)(file_t *file, const void *buf, size_t len);
    int (*read)(file_t *file, void *buf, size_t len);
};

struct vnode_operations {
    int (*lookup)(struct dentry *dir_node,
                  struct dentry **target,
                  const char *component_name);
    int (*create)(struct dentry *dir_node,
                  struct dentry **target,
                  const char *component_name);
};

int register_filesystem(struct filesystem *fs);
file_t *vfs_open(const char *pathname, int flags);
int vfs_close(file_t *file);
int vfs_write(file_t *file, const void *buf, size_t len);
int vfs_read(file_t *file, void *buf, size_t len);
dir_t *vfs_opendir(char *pathname);
dirent_t *vfs_readdir(dir_t *dir, dirent_t *entry);
void vfs_closedir(dir_t *dir);

#endif