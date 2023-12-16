#ifndef _MOUNT_H
#define _MOUNT_H

#include <include/vfs.h>

struct vfsmount {
    struct dentry *mnt_root;     // root directory of mounted file system
    struct super_block *mnt_sb;  // superblock of file system
};

struct super_block {
    struct dentry *s_root;      // dentry for root directory
    struct list_head s_mounts;  // list of mount
};

struct mount {
    struct vfsmount mnt;
    struct mount *mnt_parent;
    struct dentry *mnt_mountpoint;
    struct list_head mnt_instance;
};

struct dentry *mount_bdev(struct filesystem *fs,
                          const char *dev_name,
                          int (*fill_super)(struct super_block *, void *),
                          void *data);
int do_mount(const char *device,
             const char *mountpoint,
             const char *filesystem);

#endif