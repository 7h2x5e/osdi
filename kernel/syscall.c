#include <include/exc.h>
#include <include/peripherals/uart.h>
#include <include/syscall.h>
#include <include/task.h>
#include <include/types.h>
#include <include/utils.h>

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
        ret = sys_fork();
        break;
    case SYS_exit:
        ret = sys_exit();
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
    task_t *task = get_current();
    return (int64_t) task->tid;
}

int64_t sys_exec(struct TrapFrame *tf)
{
    task_t *task = get_current();
    void *ustack = &ustack_pool[task->tid + 1][0];
    /*
     * User task will resume from 'func' not where to call
     * exec and use new stack pointer.
     */
    tf->elr_el1 = tf->x[0];
    tf->sp = (uint64_t) ustack;
    return 0;
}

int64_t sys_fork()
{
    // to do
    return 0;
}

int64_t sys_exit()
{
    // to do
    return 0;
}