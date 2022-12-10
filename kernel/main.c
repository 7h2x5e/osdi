#include <include/fb.h>
#include <include/irq.h>
#include <include/peripherals/timer.h>
#include <include/peripherals/uart.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/demo.h>

void main()
{
    uart_init(115200, UART_INTERRUPT_MODE);
    uart_flush();
    fb_init();
    fb_showpicture();
    init_task();
    privilege_task_create(&zombie_reaper);
    privilege_task_create(&required_3_1);
    core_timer_enable();
    enable_irq();
    idle();
}