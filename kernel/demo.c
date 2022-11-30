#include <include/demo.h>
#include <include/string.h>
#include <include/types.h>

// kernel task
#include <include/irq.h>
#include <include/printk.h>
#include <include/sched.h>
#include <include/task.h>

// user task
#include <include/signal.h>
#include <include/stdio.h>
#include <include/syscall.h>

// kernel task
void task1()
{
    while (1) {
        printk("[PID %d] Kernel task 1...\n", do_get_taskid());
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        enable_irq();  // Enable IRQ if it returns from IRQ handler
    }
}

void task2()
{
    while (1) {
        printk("[PID %d] Kernel task 2...\n", do_get_taskid());
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        reschedule();
        enable_irq();
    }
}

void task3()
{
    do_exec(&utask1);
}

// user task
void utask1()
{
    char buf[256];
    do {
        printf("[PID %d] input string:\n", get_taskid());
        if (!fgets(buf, 256))
            continue;
        printf("[PID %d] your string: %s\n", get_taskid(), buf);
    } while (1);
    exit();
}