#include <include/tmpfs.h>
#include <include/vfs.h>
#include <include/types.h>
#include <include/error.h>
#include <include/slab.h>
#include <include/string.h>
#include <include/mount.h>

static int setup_vnode(struct vnode *node);

static int v_lookup(dentry_t *dir,
                    dentry_t **target,
                    const char *component_name)
{
    if (0 == strcmp(".", component_name)) {
        *target = dir;
        return 0;
    } else if (0 == strcmp("..", component_name)) {
        if (dir->parent != NULL) {
            *target = dir->parent;
        }
        return 0;
    }

    if (dir->d_mount != NULL) {
        return dir->d_mount->vnode->v_ops->lookup(dir->d_mount, target,
                                                  component_name);
    }

    dentry_t *entry;
    list_for_each_entry(entry, &dir->l_head, c_head)
    {
        if (!strcmp(entry->name, component_name)) {
            *target = entry;
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

    // check if duplicate
    dentry_t *tmp;
    list_for_each_entry(tmp, &dir_node->l_head, c_head)
    {
        if (!strcmp(tmp->name, component_name)) {
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
    if (!new->vnode || setup_vnode(new->vnode)) {
        goto _v_create_fail;
    }
    new->parent = dir_node;
    new->d_sb = NULL;
    new->d_mount = NULL;
    new->p_mount = NULL;
    INIT_LIST_HEAD(&new->l_head);
    INIT_LIST_HEAD(&new->c_head);

    // set up inode
    tmpfs_node_t *n = (tmpfs_node_t *) kzalloc(sizeof(tmpfs_node_t));
    if (!n) {
        goto _v_create_fail;
    }
    new->inode = (struct inode *) &n->inode;
    new->inode->size = 0;

    // put new file/directory in parent
    list_add_tail(&new->c_head, &dir_node->l_head);

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
    tmpfs_node_t *n =
        container_of(file->dentry->inode, struct tmpfs_node, inode);
    struct inode *i = &n->inode;
    uint8_t *data = n->buffer;
    int count = MIN(len, TMPFS_FIEL_BUFFER_MAX_LEN - file->f_pos);
    memcpy(data + file->f_pos, buf, count);
    file->f_pos += count;
    if (file->f_pos > i->size) {
        i->size = file->f_pos;  // update file length
    }
    return count;
}

static int f_read(file_t *file, void *buf, size_t len)
{
    tmpfs_node_t *n =
        container_of(file->dentry->inode, struct tmpfs_node, inode);
    struct inode *i = &n->inode;
    uint8_t *data = n->buffer;
    int count = MIN(len, i->size - file->f_pos);
    memcpy(buf, data + file->f_pos, count);
    file->f_pos += count;
    return count;
}

static int tmpfs_fill_super(struct super_block *sb, void *data)
{
    if (!sb)
        goto _error;

    // set up dentry of root directory
    dentry_t *root = (dentry_t *) kzalloc(sizeof(dentry_t));
    if (!root)
        goto _error;
    root->name = "/";
    root->flag = DIRECTORY;
    root->vnode = (struct vnode *) kzalloc(sizeof(struct vnode));
    if (!root->vnode || setup_vnode(root->vnode))
        goto _error;
    root->parent = NULL;
    root->d_sb = sb;
    root->d_mount = NULL;
    root->p_mount = NULL;
    INIT_LIST_HEAD(&root->l_head);
    INIT_LIST_HEAD(&root->c_head);

    // set up inode
    tmpfs_node_t *n = (tmpfs_node_t *) kzalloc(sizeof(tmpfs_node_t));
    if (!n) {
        goto _error;
    }
    root->inode = (struct inode *) &n->inode;
    root->inode->size = 0;

    // set up superblock
    sb->s_root = root;
    INIT_LIST_HEAD(&sb->s_mounts);
    return 0;

_error:
    if (root) {
        kfree(root->vnode);
    }
    kfree(root);
    return -1;
}

static int setup_vnode(struct vnode *node)
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

    return 0;

_setup_vnode_fail:
    kfree(node->v_ops);
    kfree(node->f_ops);
    return -1;
}

struct dentry *tmpfs_mount(struct filesystem *fs, const char *name, void *data)
{
    return mount_bdev(fs, name, tmpfs_fill_super, data);
}

void tmpfs_init()
{
    struct filesystem *fs = kmalloc(sizeof(struct filesystem));
    fs->mount = tmpfs_mount;
    fs->name = "tmpfs";
    fs->sb = NULL;
    register_filesystem(fs);
}