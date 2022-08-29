#ifndef SYSCALL_H
#define SYSCALL_H

#include <include/exc.h>
#include <include/utils.h>

enum {
    SYS_reset,
    SYS_cancel_reset,
    SYS_get_timestamp,
    SYS_uart_read,
    SYS_uart_write,
    SYS_exec,
    SYS_fork,
    SYS_exit
};

void syscall_handler(struct TrapFrame *tf);

/* for user space */
int64_t reset(uint64_t);
int64_t cancel_reset();
int64_t get_timestamp(struct TimeStamp *);
int64_t uart_read(void *, size_t);
int64_t uart_write(void *, size_t);
int64_t exec();
int64_t fork();
int64_t exit();

/* wrapper */
int64_t sys_reset(uint64_t);
int64_t sys_cancel_reset();
int64_t sys_get_timestamp(struct TimeStamp *);
int64_t sys_uart_read(void *, size_t);
int64_t sys_uart_write(void *, size_t);
int64_t sys_exec();
int64_t sys_fork();
int64_t sys_exit();

#endif
