#include <include/fatfs.h>
#include <include/vfs.h>
#include <include/types.h>
#include <include/error.h>
#include <include/slab.h>
#include <include/string.h>
#include <include/mount.h>
#include <include/list.h>
#include <include/mbr.h>
#include <include/fat.h>
#include <include/sd.h>

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

    if (list_empty(&dir->l_head)) {
        void *buffer = kmalloc(bytesPerCluster);
        struct list_head entry_list;
        struct fat_entry *pos, *tmp;
        fatfs_node_t *n = container_of(dir->inode, fatfs_node_t, inode);
        readblock(clusterAddress(n->cluster, false) / SECTOR_SIZE, buffer);
        get_entries(buffer, bytesPerCluster, &entry_list);

        list_for_each_entry_safe(pos, tmp, &entry_list, head)
        {
            dentry_t *new = kzalloc(sizeof(dentry_t));
            if (pos->attributes == ARCHIVE) {
                new->flag = FILE;
            } else if (pos->attributes == SUBDIRECTORY) {
                new->flag = DIRECTORY;
            }
            new->vnode = (struct vnode *) kzalloc(sizeof(struct vnode));
            setup_vnode(new->vnode);
            new->parent = dir;
            new->d_sb = NULL;
            new->d_mount = NULL;
            INIT_LIST_HEAD(&new->l_head);
            INIT_LIST_HEAD(&new->c_head);

            size_t len = strlen(pos->long_name);
            if (!len) {
                new->name = get_entry_short_filename(*pos);
            } else {
                new->name = (char *) kmalloc(len + 1);
                strcpy(new->name, pos->long_name);
            }

            // set up inode
            fatfs_node_t *n = (fatfs_node_t *) kzalloc(sizeof(fatfs_node_t));
            n->inode.size = pos->size;
            n->inode.off = (pos->address - buffer);
            n->cluster = pos->cluster;
            new->inode = (struct inode *) &n->inode;

            list_add_tail(&new->c_head, &dir->l_head);
            kfree(pos->long_name);
            kfree(pos);
        }
        kfree(buffer);
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
    /* not implemented */
    return -1;
}

static int f_write(file_t *file, const void *buf, size_t len)
{
    fatfs_node_t *n =
        container_of(file->dentry->inode, struct fatfs_node, inode);
    int cluster = n->cluster, pre_cluster;
    struct inode *i = &n->inode;
    size_t begin_cluster = file->f_pos / bytesPerCluster,
           offset = file->f_pos % bytesPerCluster;

    // jump to where we want to write
    for (int i = 0; i < begin_cluster && cluster != FAT_LAST; i++) {
        cluster = nextCluster(cluster);
    }
    if (cluster == FAT_LAST) {
        return 0;
    }

    size_t write_count = len;
    while (write_count > 0 && cluster != FAT_LAST) {
        size_t count =
            MIN(bytesPerCluster - offset, write_count);  // < 512 bytes
        writeData(clusterAddress(cluster, false) + offset, (char *) buf, count);
        offset = 0;
        buf += count;
        write_count -= count;
        pre_cluster = cluster;
        cluster = nextCluster(cluster);

        // find a new cluster
        if (cluster == FAT_LAST && write_count > 0) {
            uint32_t fatEntry, addr;
            unsigned int new_cluster = pre_cluster + 1;

            while ((addr = fatStart + new_cluster * sizeof(fatEntry)) <
                   dataStart) {
                readData(addr, (char *) &fatEntry, sizeof(fatEntry));

                if (fatEntry == 0x00000000) {
                    // old FAT entry
                    fatEntry = 0x0FFFFFFF & new_cluster;
                    writeData(fatStart + pre_cluster * sizeof(fatEntry),
                              (char *) &fatEntry, sizeof(fatEntry));
                    // new FAT entry
                    fatEntry = 0x0FFFFFF8;  // In use (end of chain)
                    writeData(addr, (char *) &fatEntry, sizeof(fatEntry));
                    cluster = new_cluster;
                    break;
                }

                new_cluster++;
            }

            // no available FAT entry
            if (addr >= dataStart) {
                break;
            }
        }
    }

    file->f_pos += (len - write_count);
    if (file->f_pos > i->size) {
        i->size = file->f_pos;

        // write new file size to the file's metadata
        void *buffer = kmalloc(bytesPerCluster);
        dentry_t *p_dentry = file->dentry->parent;
        fatfs_node_t *p_node =
            container_of(p_dentry->inode, fatfs_node_t, inode);
        readblock(clusterAddress(p_node->cluster, false) / SECTOR_SIZE, buffer);
        ((sfn_t *) (buffer + i->off))->size = i->size;
        writeblock(clusterAddress(p_node->cluster, false) / SECTOR_SIZE,
                   buffer);
        kfree(buffer);
    }
    return (len - write_count);
}

static int f_read(file_t *file, void *buf, size_t len)
{
    fatfs_node_t *n =
        container_of(file->dentry->inode, struct fatfs_node, inode);
    int cluster = n->cluster;
    struct inode *i = &n->inode;
    size_t size = MIN(len, i->size - file->f_pos),
           begin_cluster = file->f_pos / bytesPerCluster,
           offset = file->f_pos % bytesPerCluster;

    if (!size) {
        return 0;
    }

    // jump to where we want to read
    for (int i = 0; i < begin_cluster && cluster != FAT_LAST; i++) {
        cluster = nextCluster(cluster);
    }
    if (cluster == FAT_LAST) {
        return 0;
    }

    size_t read_count = size;
    while (read_count > 0 && cluster != FAT_LAST) {
        size_t count =
            MIN(bytesPerCluster - offset, read_count);  // < 512 bytes
        readData(clusterAddress(cluster, false) + offset, buf, count);
        offset = 0;
        buf += count;
        read_count -= count;
        cluster = nextCluster(cluster);
    }

    file->f_pos += (size - read_count);
    return (size - read_count);
}

static int fatfs_fill_super(struct super_block *sb, void *data)
{
    char buffer[512];
    uint32_t begin_sector;
    struct boot_record record;
    struct bios_parameter_block *bpb = &record.bpb;
    struct extended_bios_parameter_block *ebpb = &record.ebpb;

    readblock(0, buffer);
    if (parse_mbr((struct mbr *) buffer, &begin_sector)) {
        return -1;
    }
    readblock(begin_sector, &record);

    bytesPerSector = bpb->numBytesPerSector;
    sectorsPerCluster = bpb->numSectorsPerCluster;
    bytesPerCluster = bytesPerSector * sectorsPerCluster;
    rootCluster = ebpb->clusterNumRootDirectory;
    dataStart = (begin_sector + bpb->numReservedSectors +
                 bpb->numFats * ebpb->sectorsPerFat) *
                bytesPerSector;
    fatStart = (begin_sector + bpb->numReservedSectors) * bytesPerSector;

    // create root directory entry
    dentry_t *root = (dentry_t *) kzalloc(sizeof(dentry_t));
    root->name = "/";
    root->flag = DIRECTORY;
    root->vnode = (struct vnode *) kzalloc(sizeof(struct vnode));
    setup_vnode(root->vnode);
    root->parent = NULL;
    root->d_sb = sb;
    root->d_mount = NULL;
    INIT_LIST_HEAD(&root->l_head);
    INIT_LIST_HEAD(&root->c_head);

    // set up inode
    fatfs_node_t *n = (fatfs_node_t *) kzalloc(sizeof(fatfs_node_t));
    n->inode.size = 0;
    n->cluster = rootCluster;
    root->inode = (struct inode *) &n->inode;

    // set up superblock
    sb->s_root = root;
    INIT_LIST_HEAD(&sb->s_mounts);
    return 0;
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

struct dentry *fatfs_mount(struct filesystem *fs, const char *name, void *data)
{
    return mount_bdev(fs, name, fatfs_fill_super, data);
}

void fatfs_init()
{
    struct filesystem *fs = kmalloc(sizeof(struct filesystem));
    fs->mount = fatfs_mount;
    fs->name = "fatfs";
    fs->sb = NULL;
    register_filesystem(fs);
}