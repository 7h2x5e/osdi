#include <include/fb.h>
#include <include/irq.h>
#include <include/peripherals/timer.h>
#include <include/peripherals/uart.h>
#include <include/sched.h>
#include <include/task.h>

void main()
{
    uart_init(115200, UART_POLLING_MODE);
    uart_flush();
    fb_init();
    fb_showpicture();
    init_task();
    privilege_task_create(&zombie_reaper);
    privilege_task_create(&task1);
    privilege_task_create(&task2);
    privilege_task_create(&task3);
    core_timer_enable();
    enable_irq();
    while (1) {
        schedule();
    }
}