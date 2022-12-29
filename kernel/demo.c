#include <include/demo.h>
#include <include/string.h>
#include <include/types.h>

// kernel task
#include <include/irq.h>
#include <include/printk.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/mm.h>

// user task
#include <include/signal.h>
#include <include/stdio.h>
#include <include/syscall.h>

// kernel task
void required_3_5()
{
    extern char _binary_user_user_img_start;
    extern char _binary_user_user_img_size;
    void *start = (void *) &_binary_user_user_img_start;
    ssize_t size = (ssize_t) &_binary_user_user_img_size;
    do_exec(start, size, (uint64_t) 0x0);
}
