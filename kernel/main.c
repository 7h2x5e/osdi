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
    for (int i = 0; i < 3; ++i) {  // N should > 2
        privilege_task_create(foo);
    }
    core_timer_enable();
    enable_irq();
    idle();
}