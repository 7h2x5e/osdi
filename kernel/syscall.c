#include <include/exc.h>
#include <include/peripherals/uart.h>
#include <include/signal.h>
#include <include/syscall.h>
#include <include/task.h>
#include <include/types.h>
#include <include/utils.h>
#include <include/mman.h>
#include <include/vfs.h>
#include <include/mount.h>
#include <include/string.h>

void syscall_handler(struct TrapFrame *tf)
{
    uint64_t syscall_num = tf->x[8];
    int64_t ret = 0;

    switch (syscall_num) {
    case SYS_reset:
        ret = sys_reset(tf->x[0]);
        break;
    case SYS_cancel_reset:
        ret = sys_cancel_reset();
        break;
    case SYS_get_timestamp:
        ret = sys_get_timestamp((struct TimeStamp *) tf->x[0]);
        break;
    case SYS_uart_read:
        ret = sys_uart_read((void *) tf->x[0], (size_t) tf->x[1]);
        break;
    case SYS_uart_write:
        ret = sys_uart_write((void *) tf->x[0], (size_t) tf->x[1]);
        break;
    case SYS_get_taskid:
        ret = sys_get_taskid();
        break;
    case SYS_exec:
        ret = sys_exec(tf);
        break;
    case SYS_fork:
        ret = sys_fork(tf);
        break;
    case SYS_exit:
        ret = sys_exit();
        break;
    case SYS_kill:
        ret = sys_kill((pid_t) tf->x[0], (int32_t) tf->x[1]);
        break;
    case SYS_mmap:
        ret = sys_mmap(tf);
        break;
    case SYS_open:
        ret = sys_open((char *) tf->x[0], (int32_t) tf->x[1]);
        break;
    case SYS_close:
        ret = sys_close((int32_t) tf->x[0]);
        break;
    case SYS_read:
        ret =
            sys_read((int32_t) tf->x[0], (void *) tf->x[1], (size_t) tf->x[2]);
        break;
    case SYS_write:
        ret =
            sys_write((int32_t) tf->x[0], (void *) tf->x[1], (size_t) tf->x[2]);
        break;
    case SYS_mkdir:
        ret = sys_mkdir((char *) tf->x[0]);
        break;
    case SYS_chdir:
        ret = sys_chdir((char *) tf->x[0]);
        break;
    case SYS_getcwd:
        ret = sys_getcwd((char *) tf->x[0], (size_t) tf->x[1]);
        break;
    case SYS_mount:
        ret = sys_mount((const char *) tf->x[0], (const char *) tf->x[1],
                        (const char *) tf->x[2]);
        break;
    case SYS_opendir:
        ret = sys_opendir((char *) tf->x[0], (dir_t **) tf->x[1]);
        break;
    case SYS_readdir:
        ret =
            sys_readdir((dir_t *) tf->x[0], (char *) tf->x[1],
                        (enum node_attr_flag *) tf->x[2], (size_t *) tf->x[3]);
        break;
    case SYS_closedir:
        ret = sys_closedir((dir_t *) tf->x[0]);
        break;
    default:
    }
    tf->x[0] = (uint64_t) ret;
}

int64_t sys_reset(uint64_t tick)
{
    return do_reset(tick);
}

int64_t sys_cancel_reset()
{
    return do_cancel_reset();
}

int64_t sys_get_timestamp(struct TimeStamp *ts)
{
    return do_get_timestamp(ts);
}

int64_t sys_uart_read(void *buf, size_t size)
{
    return (int64_t) _uart_read(buf, size);
}

int64_t sys_uart_write(void *buf, size_t size)
{
    return (int64_t) _uart_write(buf, size);
}

int64_t sys_get_taskid()
{
    return (int64_t) do_get_taskid();
}

int64_t sys_exec(struct TrapFrame *tf)
{
    /*
     * The exec() syscall function returns only if an error has occured.
     * The return value is -1 and the user must call exit() to reclaim pages.
     */
    return do_exec(tf->x[0]);
}

int64_t sys_fork(struct TrapFrame *tf)
{
    return do_fork(tf);
}

int64_t sys_exit()
{
    /* do_exit() does not return */
    do_exit();
    return 0;
}

int64_t sys_kill(pid_t pid, int32_t sig)
{
    return (int64_t) do_kill(pid, sig);
}

int64_t sys_mmap(struct TrapFrame *tf)
{
    return (int64_t) do_mmap((void *) tf->x[0], (size_t) tf->x[1],
                             (mmap_prot_t) tf->x[2], (mmap_flags_t) tf->x[3],
                             (void *) tf->x[4], (off_t) tf->x[5]);
}

int64_t sys_open(char *pathname, int32_t flags)
{
    return (int64_t) do_open(pathname, flags);
}

int64_t sys_close(int32_t fd)
{
    return (int64_t) do_close(fd);
}

int64_t sys_write(int32_t fd, void *buf, size_t size)
{
    return (int64_t) do_write(fd, buf, size);
}

int64_t sys_read(int32_t fd, void *buf, size_t size)
{
    return (int64_t) do_read(fd, buf, size);
}

int64_t sys_mkdir(char *pathname)
{
    return (int64_t) do_mkdir(pathname);
}

int64_t sys_chdir(char *pathname)
{
    return (int64_t) do_chdir(pathname);
}

int64_t sys_getcwd(char *pathname, size_t size)
{
    return (int64_t) do_getcwd(pathname, size);
}

int64_t sys_mount(const char *device,
                  const char *mountpoint,
                  const char *filesystem)
{
    return (int64_t) do_mount(device, mountpoint, filesystem);
}

int64_t sys_opendir(char *pathname, dir_t **dir)
{
    if (!dir)
        return -1;
    *dir = vfs_opendir(pathname);
    return 0;
}

int64_t sys_readdir(dir_t *dir,
                    char *name,
                    enum node_attr_flag *flag,
                    size_t *size)
{
    if (!dir)
        return -1;
    dentry_t *entry = vfs_readdir(dir);
    if (!entry)
        return -1;
    strcpy(name, entry->name);
    *flag = entry->flag;
    *size = entry->inode->size;
    return 0;
}

int64_t sys_closedir(dir_t *dir)
{
    vfs_closedir(dir);
    return 0;
}