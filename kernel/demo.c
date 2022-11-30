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
void required_2_3()
{
    printk("Allocate 10 pages...\n");

    int i;
    void *virt_addr[10];
    for (i = 0; i < 10; ++i) {
        virt_addr[i] = page_alloc_kernel();
        printk("page start: 0x%h\n", (uintptr_t) virt_addr[i]);
    }
    for (i = 0; i < 10; ++i) {
        page_free(virt_addr[i]);
    }
    do_exit();
}

// user task