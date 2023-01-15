#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <include/printk.h>
#include <include/task.h>

#define LOG_LEVEL 3

#define KERNEL_LOG_DEBUG(M, ...) \
    printk("[kernel] pid %d: " M "\n", do_get_taskid(), ##__VA_ARGS__)
#define KERNEL_LOG_INFO(M, ...) printk("[kernel] " M "\n", ##__VA_ARGS__)
#define KERNEL_LOG_ERROR(M, ...) \
    printk("[kernel] pid %d: " M "\n", do_get_taskid(), ##__VA_ARGS__)

#if LOG_LEVEL < 3
#undef KERNEL_LOG_DEBUG
#define KERNEL_LOG_DEBUG(M, ...)
#endif

#if LOG_LEVEL < 2
#undef KERNEL_LOG_INFO
#define KERNEL_LOG_INFO(M, ...)
#endif

#if LOG_LEVEL < 1
#undef KERNEL_LOG_ERROR
#define KERNEL_LOG_ERROR(M, ...)
#endif

#endif