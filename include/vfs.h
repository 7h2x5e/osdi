#ifndef _VFS_H
#define _VFS_H

#include <include/types.h>
#include <include/list.h>

#define MAX_CHILD_DIR 10

enum { FILE_FOUND, FILE_NOT_FOUND, DIR_NOT_FOUND };

enum file_op_flag {
    O_CREAT = 0b1,
};

enum node_attr_flag {
    DIRECTORY,
    FILE,
};

struct vnode {
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
    struct super_block *d_sb;  // superblock of filesystem
    struct dentry *d_mount, *p_mount;
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

typedef struct path {
    dentry_t *dentry;
} path_t;

struct fs_struct {
    path_t pwd;
};

struct filesystem {
    const char *name;
    struct dentry *(*mount)(struct filesystem *fs,
                            const char *name,
                            void *data);
    struct list_head head;
    struct super_block *sb;
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
                  const char *component_name,
                  enum node_attr_flag flag);
};

void register_filesystem(struct filesystem *fs);
struct filesystem *find_filesystem(const char *str);
int find_dentry(const char *pathname,
                dentry_t **target,
                char last_component_name[]);
file_t *vfs_open(const char *pathname, int flags);
int vfs_close(file_t *file);
int vfs_write(file_t *file, const void *buf, size_t len);
int vfs_read(file_t *file, void *buf, size_t len);
dir_t *vfs_opendir(char *pathname);
dirent_t *vfs_readdir(dir_t *dir, dirent_t *entry);
void vfs_closedir(dir_t *dir);
int vfs_mkdir(char *pathname);
int vfs_chdir(char *pathname);
int vfs_getcwd(char *pathname, size_t size);
void vfs_test();

/* for syscall */
int32_t do_open(char *pathname, int32_t flags);
int32_t do_close(int32_t fd);
ssize_t do_write(int32_t fd, void *buf, size_t size);
ssize_t do_read(int32_t fd, void *buf, size_t size);
int32_t do_mkdir(char *pathname);
int32_t do_chdir(char *pathname);
int32_t do_getcwd(char *pathname, size_t size);

extern struct dentry *root_dir;

#endif