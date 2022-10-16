#include <include/fb.h>
#include <include/irq.h>
#include <include/peripherals/timer.h>
#include <include/peripherals/uart.h>
#include <include/sched.h>
#include <include/task.h>

void main()
{
    uart_init(115200, UART_INTERRUPT_MODE);
    uart_flush();
    fb_init();
    fb_showpicture();
    init_task();
    privilege_task_create(&zombie_reaper);  // 1
    privilege_task_create(&user_test1);     // 2
    privilege_task_create(&user_test2);     // 3
    core_timer_enable();
    enable_irq();
    idle();
}