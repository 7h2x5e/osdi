#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <include/printk.h>
#include <include/task.h>

#define LOG_LEVEL 0

#define KERNEL_LOG_TRACE(M, ...)                      \
    printk("\33[0;33m[kernel][trace][%d]\33[0;39m " M \
           " \33[0;33m[in %s at %s:%d]\33[0m\n",      \
           do_get_taskid(), ##__VA_ARGS__, __FUNCTION__, __FILE__, __LINE__)
#define KERNEL_LOG_DEBUG(M, ...)                                   \
    printk("\33[0;32m[kernel][debug][%d]\33[0;39m " M " \33[0m\n", \
           do_get_taskid(), ##__VA_ARGS__)
#define KERNEL_LOG_INFO(M, ...) printk(M "\n", ##__VA_ARGS__)

#if LOG_LEVEL < 3
#undef KERNEL_LOG_TRACE
#define KERNEL_LOG_TRACE(M, ...)
#endif

#if LOG_LEVEL < 2
#undef KERNEL_LOG_DEBUG
#define KERNEL_LOG_DEBUG(M, ...)
#endif

#if LOG_LEVEL < 1
#undef KERNEL_LOG_INFO
#define KERNEL_LOG_INFO(M, ...)
#endif

#endif