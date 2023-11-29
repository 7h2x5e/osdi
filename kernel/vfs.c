#include <include/vfs.h>
#include <include/error.h>
#include <include/string.h>
#include <include/slab.h>
#include <include/kernel_log.h>
#include <include/assert.h>
#include <include/task.h>

static struct mount *rootfs = NULL;

enum { FILE_FOUND, FILE_NOT_FOUND, DIR_NOT_FOUND };

int register_filesystem(struct filesystem *fs)
{
    if (!fs)
        return E_INVAL;

    // mount tmpfs to rootfs
    if (strcmp(fs->name, "tmpfs") == 0) {
        if (!rootfs) {
            rootfs = (struct mount *) kzalloc(sizeof(struct mount));
            fs->setup_mount(fs, rootfs);
        }
        return 0;
    }

    return E_INVAL;
}

static const char *find_component_name(const char *str, char *component_name)
{
    size_t len, i;

    if (!(len = strlen(str))) {
        component_name[0] = 0;
        return str;  // empty string
    }

    // locate first '/', assume str = "AAA/BBB/CCC..."
    for (i = 0; i < len && str[i] != '/'; i++)
        ;

    if (i == len) {
        memcpy(component_name, str, i);
        component_name[i] = 0;
        return &str[i];  // empty string
    }

    memcpy(component_name, str, i);
    component_name[i] = 0;
    return &str[i + 1];  // skip '/'
}

static int find_dentry(const char *pathname,
                       dentry_t **target,
                       char last_component_name[])
{
    dentry_t *cur, *tmp;
    char component_name[256];

    if (!rootfs || !strlen(pathname) || pathname[0] != '/') {
        *target = NULL;
        return FILE_NOT_FOUND;
    }

    cur = rootfs->root_dir;
    pathname = find_component_name(pathname + 1, component_name);

    while ((cur->flag == DIRECTORY) && (0 < strlen(component_name)) &&
           (0 == cur->vnode->v_ops->lookup(cur, &tmp, component_name))) {
        cur = tmp;
        pathname = find_component_name(pathname, component_name);
    }

    if (strlen(component_name) == 0) {
        strcpy(last_component_name, cur->name);
        *target = cur;
        return FILE_FOUND;
    }

    // save `component_name` content
    strcpy(last_component_name, component_name);

    // more component?
    pathname = find_component_name(pathname, component_name);

    // no, it's the last one component
    if (strlen(component_name) == 0) {
        *target = cur;
        return FILE_NOT_FOUND;
    }

    // yes
    return DIR_NOT_FOUND;
}

file_t *vfs_open(const char *pathname, int flags)
{
    int ret;
    file_t *file = NULL;
    dentry_t *target, *dentry;
    char last_component_name[256];

    ret = find_dentry(pathname, &target, last_component_name);

    if (ret == DIR_NOT_FOUND) {
        // KERNEL_LOG_INFO("VFS :: Dir %s not found", last_component_name);
        return NULL;
    } else if (ret == FILE_NOT_FOUND) {
        if (flags & O_CREAT) {
            if (0 != target->vnode->v_ops->create(target, &dentry,
                                                  last_component_name)) {
                // KERNEL_LOG_INFO("VFS :: File %s couldn't be created",
                //                 last_component_name);
                return NULL;
            }
            // KERNEL_LOG_INFO("VFS :: File %s is created",
            // last_component_name);
        } else {
            // KERNEL_LOG_INFO("VFS :: File %s not found", last_component_name);
            return NULL;
        }
    } else {
        // KERNEL_LOG_INFO("VFS :: File %s is found", last_component_name);
        dentry = target;
    }

    file = (file_t *) kzalloc(sizeof(file_t));
    file->dentry = dentry;
    file->f_pos = 0;

    return file;
}

int vfs_close(file_t *file)
{
    if (file) {
        kfree(file);
    }

    return 0;
}

int vfs_write(file_t *file, const void *buf, size_t len)
{
    return file->dentry->vnode->f_ops->write(file, buf, len);
}

int vfs_read(file_t *file, void *buf, size_t len)
{
    return file->dentry->vnode->f_ops->read(file, buf, len);
}

dir_t *vfs_opendir(char *pathname)
{
    dir_t *dir = NULL;
    file_t *file = vfs_open(pathname, 0);
    if (file && file->dentry->flag == DIRECTORY) {
        dir = kmalloc(sizeof(dir_t));
        dir->idx = 0;
        dir->dentry = file->dentry;
    }
    vfs_close(file);
    return dir;
}

dirent_t *vfs_readdir(dir_t *dir, dirent_t *entry)
{
    if (!dir || !entry || dir->dentry->child_amount <= dir->idx)
        return NULL;
    entry->dentry = dir->dentry->child[dir->idx++];
    return entry;
}

void vfs_closedir(dir_t *dir)
{
    kfree(dir);
}

static bool valid_fd(int32_t fd)
{
    return (fd >= 0 && fd < MAX_FILE_DESCRIPTOR);
}

static int32_t get_unused_fd()
{
    task_t *task = (task_t *) get_current();
    for (int i = 0; i < MAX_FILE_DESCRIPTOR; ++i) {
        if (!task->fdt[i])
            return i;
    }
    return -1;
}

int32_t do_open(char *pathname, int32_t flags)
{
    task_t *task = (task_t *) get_current();
    file_t *file = vfs_open(pathname, flags);
    if (!file)
        return -1;
    for (int32_t i = 0; i < MAX_FILE_DESCRIPTOR; ++i) {
        if (task->fdt[i] == file)
            return i;  // return exist fd
    }
    int32_t fd = get_unused_fd();
    if (fd == -1)
        return -1;
    task->fdt[fd] = file;
    return fd;
}

int32_t do_close(int32_t fd)
{
    task_t *task = (task_t *) get_current();
    file_t *file;
    if (!valid_fd(fd) || !(file = task->fdt[fd]))
        return -1;
    vfs_close(file);
    task->fdt[fd] = NULL;
    return 0;
}

ssize_t do_write(int32_t fd, void *buf, size_t size)
{
    task_t *task = (task_t *) get_current();
    if (!valid_fd(fd) || !buf)
        return -1;
    return (ssize_t) vfs_write(task->fdt[fd], (const void *) buf, size);
}

ssize_t do_read(int32_t fd, void *buf, size_t size)
{
    task_t *task = (task_t *) get_current();
    if (!valid_fd(fd) || !buf)
        return -1;
    return (ssize_t) vfs_read(task->fdt[fd], buf, size);
}

void vfs_test()
{
    file_t *file;
    uint8_t buf[64], testdata[64];  // assume max file size in tmpfs = 64 bytes
    size_t count;
    dirent_t entry;
    dir_t *dir;

    for (int i = 0; i < 64; i++)
        testdata[i] = i;

    KERNEL_LOG_INFO("<-- VFS API Test Start -->");

    KERNEL_LOG_INFO("==> Open root directory: /");
    file = vfs_open("/", 0);
    assert(file != NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Parent directory not exist: %s", "/komica/komica.txt");
    file = vfs_open("/komica/2", 0);
    assert(file == NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> File not exist: %s", "/komica.txt");
    file = vfs_open("/komica.txt", 0);
    assert(file == NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Open and create file: %s", "/komica.txt");
    file = vfs_open("/komica.txt", O_CREAT);
    assert(file != NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Write to file: %s", "/komica.txt");
    file = vfs_open("/komica.txt", 0);
    assert(file != NULL);
    count = vfs_write(file, testdata, 32);
    assert(count == 32);
    count = vfs_write(file, testdata + 32, 32);
    assert(count == 32);
    count = vfs_write(file, testdata + 64, 32);
    assert(count == 0);  // full
    vfs_close(file);

    KERNEL_LOG_INFO("==> Read from file: %s", "/komica.txt");
    file = vfs_open("/komica.txt", 0);
    assert(file != NULL);
    count = vfs_read(file, buf, 128);
    assert(count == 64);
    assert(0 == memcmp(testdata, buf, 64));
    vfs_close(file);

    KERNEL_LOG_INFO("==> List all entry in root directory");
    file = vfs_open("/komica2.txt", O_CREAT);
    vfs_close(file);
    file = vfs_open("/komica3.txt", O_CREAT);
    vfs_close(file);
    dir = vfs_opendir("/");
    assert(dir != NULL);
    KERNEL_LOG_INFO("Type\tFilename");
    while (vfs_readdir(dir, &entry) != NULL) {
        KERNEL_LOG_INFO("%c\t%s", entry.dentry->flag == DIRECTORY ? 'D' : 'F',
                        entry.dentry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("<-- VFS API Test End -->");
}
