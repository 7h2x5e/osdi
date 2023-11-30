#include <include/tmpfs.h>
#include <include/vfs.h>
#include <include/types.h>
#include <include/error.h>
#include <include/slab.h>
#include <include/string.h>

static int setup_vnode(struct vnode *node, struct mount *mount);

static int v_lookup(dentry_t *dir,
                    dentry_t **target,
                    const char *component_name)
{
    for (int i = 0; i < dir->child_amount; i++) {
        if (0 == strcmp(dir->child[i]->name, component_name)) {
            *target = dir->child[i];
            return 0;
        }
    }
    *target = NULL;
    return -1;
}

static int v_create(dentry_t *dir_node,
                    dentry_t **target,
                    const char *component_name,
                    enum node_attr_flag flag)
{
    if (dir_node->flag != DIRECTORY) {
        return -1;
    }

    if (dir_node->child_amount >= MAX_CHILD_DIR) {
        return -1;
    }

    // check if duplicate
    for (int i = 0; i < dir_node->child_amount; i++) {
        if (0 == strcmp(dir_node->child[i]->name, component_name)) {
            return -1;
        }
    }

    // create file
    dentry_t *new = (dentry_t *) kzalloc(sizeof(dentry_t));
    if (!new) {
        goto _v_create_fail;
    }
    new->name = (char *) kzalloc(sizeof(char) * (strlen(component_name) + 1));
    if (!new->name) {
        goto _v_create_fail;
    }
    strcpy(new->name, component_name);
    new->flag = flag;
    new->vnode = (struct vnode *) kzalloc(sizeof(struct vnode));
    if (!new->vnode || setup_vnode(new->vnode, NULL)) {
        goto _v_create_fail;
    }
    new->child_amount = 0;
    new->parent = dir_node;
    memset(new->child, 0, sizeof(new->child));

    if (new->flag == FILE) {
        // set new file size as 0
        new->internal = (void *) kzalloc(sizeof(tmpfs_node_t));
        if (!new->internal) {
            goto _v_create_fail;
        }
        ((tmpfs_node_t *) (new->internal))->file_length = 0;
    }

    // put new file/directory in parent
    dir_node->child[dir_node->child_amount++] = new;

    *target = new;
    return 0;

_v_create_fail:
    if (new) {
        kfree(new->vnode);
        kfree(new->name);
    }
    kfree(new);
    return -1;
}

static int f_write(file_t *file, const void *buf, size_t len)
{
    size_t *file_length_p =
        &((tmpfs_node_t *) file->dentry->internal)->file_length;
    uint8_t *data = ((tmpfs_node_t *) file->dentry->internal)->buffer;
    int count = MIN(len, TMPFS_FIEL_BUFFER_MAX_LEN - file->f_pos);
    memcpy(data + file->f_pos, buf, count);
    file->f_pos += count;
    if (file->f_pos > *file_length_p) {
        *file_length_p = file->f_pos;  // update file length
    }
    return count;
}

static int f_read(file_t *file, void *buf, size_t len)
{
    size_t *file_length_p =
        &((tmpfs_node_t *) file->dentry->internal)->file_length;
    uint8_t *data = ((tmpfs_node_t *) file->dentry->internal)->buffer;
    int count = MIN(len, *file_length_p - file->f_pos);
    memcpy(buf, data + file->f_pos, count);
    file->f_pos += count;
    return count;
}

static int setup_mount(struct filesystem *fs, struct mount *mount)
{
    if (!fs || !mount)
        return E_INVAL;

    mount->fs = fs;

    // set up root dir
    mount->root_dir = (dentry_t *) kzalloc(sizeof(dentry_t));
    if (!mount->root_dir) {
        goto _setup_mount_fail;
    }
    mount->root_dir->name = "/";
    mount->root_dir->flag = DIRECTORY;
    mount->root_dir->vnode = (struct vnode *) kzalloc(sizeof(struct vnode));
    if (!mount->root_dir->vnode || setup_vnode(mount->root_dir->vnode, mount))
        goto _setup_mount_fail;
    mount->root_dir->child_amount = 0;
    mount->root_dir->parent = NULL;
    memset(mount->root_dir->child, 0, sizeof(mount->root_dir->child));

    return 0;

_setup_mount_fail:
    if (mount->root_dir) {
        kfree(mount->root_dir->vnode);
    }
    kfree(mount->root_dir);
    return E_FAULT;
}

static int setup_vnode(struct vnode *node, struct mount *mount)
{
    node->v_ops =
        (struct vnode_operations *) kzalloc(sizeof(struct vnode_operations));
    node->f_ops =
        (struct file_operations *) kzalloc(sizeof(struct file_operations));
    if (!node->v_ops || !node->f_ops) {
        goto _setup_vnode_fail;
    }

    node->v_ops->lookup = v_lookup;
    node->v_ops->create = v_create;
    node->f_ops->write = f_write;
    node->f_ops->read = f_read;
    node->mount = mount;

    return 0;

_setup_vnode_fail:
    kfree(node->v_ops);
    kfree(node->f_ops);
    return -1;
}

void tmpfs_init()
{
    struct filesystem *fs = kmalloc(sizeof(struct filesystem));
    fs->name = "tmpfs";
    fs->setup_mount = setup_mount;

    register_filesystem(fs);
}
