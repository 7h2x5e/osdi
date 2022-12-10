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
void required_3_1()
{
    do_exec(required_3_1_user);
}

// user task
void required_3_1_user()
{
    while (1) {
        // user library hasn't implemented
        // printf("Hello world\n");
        // fork();
    }
}