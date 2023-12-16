#include <include/mount.h>
#include <include/slab.h>
#include <include/string.h>
#include <include/tmpfs.h>
#include <include/vfs.h>

struct dentry *mount_bdev(struct filesystem *fs,
                          const char *dev_name,
                          int (*fill_super)(struct super_block *, void *),
                          void *data)
{
    if (fs->sb) {
        return fs->sb->s_root;
    }

    // create superblock and root directory's dentry
    struct super_block *s =
        (struct super_block *) kzalloc(sizeof(struct super_block));
    if (!s || fill_super(s, data)) {
        kfree(s);
        return NULL;
    }

    fs->sb = s;
    return s->s_root;
}

static struct dentry *mount_fs(struct filesystem *fs,
                               const char *name,
                               void *data)
{
    struct dentry *root = fs->mount(fs, name, data);
    if (!root) {
        return NULL;
    }
    return root;
}

static struct vfsmount *vfs_kern_mount(struct filesystem *type,
                                       const char *name,
                                       void *data)
{
    struct mount *mnt;
    struct dentry *root;

    mnt = (struct mount *) kzalloc(sizeof(struct mount));
    if (!mnt)
        goto _error;

    // create dentry of root directory and superblock
    root = mount_fs(type, name, data);
    if (!root)
        goto _error;

    // create relationship among mount, dentry of root directory and superblock
    mnt->mnt.mnt_root = root;
    mnt->mnt.mnt_sb = root->d_sb;
    mnt->mnt_mountpoint = mnt->mnt.mnt_root;
    mnt->mnt_parent = mnt;
    list_add_tail(&mnt->mnt_instance,
                  &root->d_sb->s_mounts);  // superblock list
    return &mnt->mnt;

_error:
    if (mnt) {
        kfree(mnt);
    }
    return NULL;
}

int do_mount(const char *device, const char *mountpoint, const char *filesystem)
{
    void *data = NULL;
    dentry_t *target;
    char last_component_name[256];

    struct filesystem *fs = find_filesystem(filesystem);
    if (!fs) {
        return -1;
    }

    struct vfsmount *mnt = vfs_kern_mount(fs, device, data);
    if (!strcmp(mountpoint, "/")) {
        root_dir = mnt->mnt_root;
        return 0;
    } else if (find_dentry(mountpoint, &target, last_component_name) !=
                   FILE_FOUND ||
               target->flag != DIRECTORY) {
        return -1;
    }
    target->d_mount = mnt->mnt_root;
    mnt->mnt_root->parent = target->parent;
    mnt->mnt_root->p_mount = target;

    return 0;
}
