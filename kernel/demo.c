#include <include/demo.h>
#include <include/string.h>
#include <include/types.h>

// kernel task
#include <include/irq.h>
#include <include/kernel_log.h>
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
    extern char _binary_user_user_elf_start;
    uint64_t start = (uint64_t) &_binary_user_user_elf_start;
    if (-1 == do_exec((uint64_t) start)) {
        do_exit();
    }
}
