#include "fb.h"
#include "irq.h"
#include "peripherals/uart.h"
#include "sched.h"

void main()
{
    uart_init(115200, 0);
    uart_flush();
    fb_init();
    fb_showpicture();
    init_task();
    privilege_task_create(&task1);
    privilege_task_create(&task2);
    privilege_task_create(&task3);
    core_timer_enable();
    enable_irq();
    while (1) {
        schedule();
    }
    __builtin_unreachable();
}