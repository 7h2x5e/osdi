#include <include/tmpfs.h>
#include <include/vfs.h>
#include <include/types.h>
#include <include/error.h>
#include <include/slab.h>

static int v_lookup(struct vnode *dir_node,
                    struct vnode **target,
                    const char *component_name)
{
    return 0;
}
static int v_create(struct vnode *dir_node,
                    struct vnode **target,
                    const char *component_name)
{
    return 0;
}

static int f_write(struct file *file, const void *buf, size_t len)
{
    return 0;
}

static int f_read(struct file *file, void *buf, size_t len)
{
    return 0;
}

static int setup_mount(struct filesystem *fs, struct mount *mount)
{
    if (!fs || !mount)
        return E_INVAL;

    mount->fs = fs;
    mount->root = (struct vnode *) kzalloc(sizeof(struct vnode));
    if (!mount->root)
        goto _setup_mount_fail;

    mount->root->v_ops =
        (struct vnode_operations *) kzalloc(sizeof(struct vnode_operations));
    if (!mount->root->v_ops)
        goto _setup_mount_fail;
    mount->root->v_ops->lookup = v_lookup;
    mount->root->v_ops->create = v_create;

    mount->root->f_ops =
        (struct file_operations *) kzalloc(sizeof(struct file_operations));
    if (!mount->root->f_ops)
        goto _setup_mount_fail;
    mount->root->f_ops->write = f_write;
    mount->root->f_ops->read = f_read;

    return 0;

_setup_mount_fail:
    if (mount->root) {
        kfree(mount->root->v_ops);
        kfree(mount->root->f_ops);
        kfree(mount->root);
    }
    return E_FAULT;
}

void tmpfs_init()
{
    struct filesystem *fs = kmalloc(sizeof(struct filesystem));
    fs->name = "tmpfs";
    fs->setup_mount = setup_mount;

    register_filesystem(fs);
}