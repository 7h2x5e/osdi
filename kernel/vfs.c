#include <include/vfs.h>
#include <include/error.h>
#include <include/string.h>
#include <include/slab.h>
#include <include/kernel_log.h>
#include <include/assert.h>
#include <include/task.h>
#include <include/types.h>
#include <include/stdio.h>
#include <include/slab.h>
#include <include/mount.h>
#include <include/list.h>

struct dentry *root_dir = NULL;
static LIST_HEAD(filesystem_list);

void register_filesystem(struct filesystem *fs)
{
    if (!fs)
        return;
    list_add_tail(&fs->head, &filesystem_list);
}

struct filesystem *find_filesystem(const char *str)
{
    struct filesystem *fs;
    list_for_each_entry(fs, &filesystem_list, head)
    {
        if (!strcmp(fs->name, str)) {
            return fs;
        }
    }
    return NULL;
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

int find_dentry(const char *pathname,  // relative or absolute path
                dentry_t **target,
                char last_component_name[])
{
    dentry_t *cur, *tmp;
    char component_name[256], buf[256];

    if (!root_dir || !strlen(pathname)) {
        *target = NULL;
        return FILE_NOT_FOUND;
    } else if (pathname[0] != '/') {
        // convert relative path to absolute path
        vfs_getcwd(buf, sizeof(buf));
        strcpy(buf + strlen(buf), pathname);
        pathname = buf;
    }

    cur = root_dir;
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
        if (cur->d_mount) {
            *target = cur->d_mount;
        } else {
            *target = cur;
        }
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
                                                  last_component_name, FILE)) {
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
    dentry_t *target = NULL;
    char last_component_name[256], pathname2[256];

    if (file && file->dentry->flag == DIRECTORY) {
        // read all dentries of the directory
        sprintf(pathname2, "%s/%s", pathname, "__dummy__");
        find_dentry(pathname2, &target, last_component_name);

        target = file->dentry;
        dir = kmalloc(sizeof(dir_t));
        if (target->d_mount) {
            target = target->d_mount;
        }
        dir->head = dir->cur = &target->l_head;
    }
    vfs_close(file);
    return dir;
}

dentry_t *vfs_readdir(dir_t *dir)
{
    if (!dir || list_is_last(dir->cur, dir->head))
        return NULL;
    dir->cur = dir->cur->next;
    return list_entry(dir->cur, dentry_t, c_head);
}

void vfs_closedir(dir_t *dir)
{
    kfree(dir);
}

int vfs_mkdir(char *pathname)
{
    dentry_t *target, *dentry;
    char last_component_name[256];
    int ret = find_dentry(pathname, &target, last_component_name);

    if (ret == DIR_NOT_FOUND) {
        // KERNEL_LOG_INFO("mkdir: dir %s not found", last_component_name);
        return -1;
    } else if (ret == FILE_FOUND && target->flag != DIRECTORY) {
        // KERNEL_LOG_INFO("mkdir: cannot create directory ‘%s’: File exists",
        //                 target->name);
        return -1;
    } else if (ret == FILE_NOT_FOUND) {
        if (0 != target->vnode->v_ops->create(target, &dentry,
                                              last_component_name, DIRECTORY)) {
            // KERNEL_LOG_INFO("mkdir: cannot create directory ‘%s’",
            //                 last_component_name);
            return -1;
        }
    }
    return 0;
}

int vfs_chdir(char *pathname)
{
    dentry_t *target;
    char last_component_name[256];
    int ret = find_dentry(pathname, &target, last_component_name);
    if (ret == FILE_FOUND && target->flag == DIRECTORY) {
        task_t *task = (task_t *) get_current();
        task->fs.pwd.dentry = target;
        return 0;
    }
    return -1;
}

static size_t print_path(dentry_t *dentry, char *buf, size_t size)
{
    // todo: size
    if (!dentry->parent || dentry->parent == dentry) {
        return sprintf(buf, "/");
    }
    size_t count = print_path(dentry->parent, buf, size);
    if (dentry->p_mount) {
        count += sprintf(buf + count, "%s/", dentry->p_mount->name);
    } else {
        count += sprintf(buf + count, "%s/", dentry->name);
    }
    return count;
}

int vfs_getcwd(char *pathname, size_t size)
{
    task_t *task = (task_t *) get_current();
    dentry_t *dentry = task->fs.pwd.dentry;
    print_path(dentry, pathname, size);
    return 0;
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

int32_t do_mkdir(char *pathname)
{
    return vfs_mkdir(pathname);
}

int32_t do_chdir(char *pathname)
{
    return vfs_chdir(pathname);
}

int32_t do_getcwd(char *pathname, size_t size)
{
    return vfs_getcwd(pathname, size);
}

void vfs_test()
{
    file_t *file;
    uint8_t buf[64], testdata[64];  // assume max file size in tmpfs = 64 bytes
    size_t count;
    dir_t *dir;
    dentry_t *entry;

    for (int i = 0; i < 64; i++)
        testdata[i] = i;

    KERNEL_LOG_INFO("<-- VFS API Test Start -->");

    KERNEL_LOG_INFO("==> Open root directory: /");
    file = vfs_open("/", 0);
    assert(file != NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Open non-existent directory: /folder");
    file = vfs_open("/folder", 0);
    assert(file == NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Open non-existent file: /folder/file.txt");
    file = vfs_open("/folder/file.txt", 0);
    assert(file == NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Open non-existent file: /file.txt");
    file = vfs_open("/file.txt", 0);
    assert(file == NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Open and create non-existent file: /file.txt");
    file = vfs_open("/file.txt", O_CREAT);
    assert(file != NULL);
    vfs_close(file);

    KERNEL_LOG_INFO("==> Write to file: /file.txt");
    file = vfs_open("/file.txt", 0);
    assert(file != NULL);
    count = vfs_write(file, testdata, 32);
    assert(count == 32);
    count = vfs_write(file, testdata + 32, 32);
    assert(count == 32);
    count = vfs_write(file, testdata + 64, 32);
    assert(count == 0);  // full
    vfs_close(file);

    KERNEL_LOG_INFO("==> Read from file: /file.txt");
    file = vfs_open("/file.txt", 0);
    assert(file != NULL);
    count = vfs_read(file, buf, 128);
    assert(count == 64);
    assert(0 == memcmp(testdata, buf, 64));
    vfs_close(file);

    KERNEL_LOG_INFO("==> mkdir: /folder, /file.txt");
    assert(0 == vfs_mkdir("/folder"));
    assert(0 == vfs_mkdir("/folder"));
    assert(-1 == vfs_mkdir("/file.txt"));

    KERNEL_LOG_INFO("==> chdir: /folder");
    assert(0 == vfs_chdir("folder"));
    vfs_getcwd((char *) buf, sizeof(buf));
    KERNEL_LOG_INFO("current directory: %s", buf);
    KERNEL_LOG_INFO("==> chdir: /");
    assert(0 == vfs_chdir("/"));
    vfs_getcwd((char *) buf, sizeof(buf));
    KERNEL_LOG_INFO("current directory: %s", buf);

#define filetype(flag) (flag == DIRECTORY ? 'D' : 'F')

    KERNEL_LOG_INFO("==> List all files in /");
    assert((dir = vfs_opendir("/")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("==> List all files in /folder");
    assert((dir = vfs_opendir("/folder")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("==> List all files in /folder");
    assert(0 == vfs_chdir("/folder"));
    assert((dir = vfs_opendir(".")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    /* Test fatfs */
    KERNEL_LOG_INFO("==> List all files in /sdcard");
    assert((dir = vfs_opendir("/sdcard")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("==> List all files in /sdcard");
    assert(0 == vfs_chdir("/"));
    assert((dir = vfs_opendir("sdcard")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("==> List all files in /sdcard");
    assert(0 == vfs_chdir("/sdcard"));
    assert((dir = vfs_opendir(".")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("==> List all files in /");
    assert(0 == vfs_chdir(".."));
    assert((dir = vfs_opendir(".")) != NULL);
    while ((entry = vfs_readdir(dir)) != NULL) {
        KERNEL_LOG_INFO("%c\t%d\t%s", filetype(entry->flag), entry->inode->size,
                        entry->name);
    }
    vfs_closedir(dir);

    KERNEL_LOG_INFO("==> Read & write first file in /sdcard");
    vfs_chdir("/sdcard");
    dir = vfs_opendir(".");
    while ((entry = vfs_readdir(dir)) != NULL) {
        if (entry->inode->size > 0) {
            size_t o_size = entry->inode->size;
            char *bbuf = (char *) kzalloc(o_size * 2 + 1);

            file = vfs_open(entry->name, 0);
            vfs_read(file, bbuf, o_size);
            vfs_write(file, bbuf, o_size);
            vfs_close(file);

            KERNEL_LOG_INFO("%s: %s", "Filename", entry->name);
            KERNEL_LOG_INFO("%s: %d -> %d", "File size", o_size,
                            entry->inode->size);

            kfree(bbuf);
            break;
        }
    }
    vfs_closedir(dir);

#undef filetype

    KERNEL_LOG_INFO("<-- VFS API Test End -->");
}
