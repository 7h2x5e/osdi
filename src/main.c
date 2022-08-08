#include "fb.h"
#include "irq.h"
#include "peripherals/uart.h"
#include "sched.h"
#include "shell.h"

void main()
{
    uart_init(115200, 0);
    uart_flush();
    fb_init();
    fb_showpicture();
    privilege_task_create(&task1);
    privilege_task_create(&task2);
    asm volatile("msr tpidr_el1, %0" ::"r"(get_task(0)));
    context_switch(get_task(1));
    __builtin_unreachable();

    shell();
    return;
}