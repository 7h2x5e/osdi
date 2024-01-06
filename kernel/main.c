#include <include/fb.h>
#include <include/irq.h>
#include <include/peripherals/timer.h>
#include <include/peripherals/uart.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/demo.h>
#include <include/mm.h>
#include <include/mount.h>
#include <include/vfs.h>
#include <include/tmpfs.h>
#include <include/fatfs.h>
#include <include/sd.h>

void main()
{
    disable_irq();

    uart_init(115200, UART_INTERRUPT_MODE);
    uart_flush();
    fb_init();
    fb_showpicture();
    mem_init();
    sd_init();
    tmpfs_init();
    fatfs_init();
    init_task();
    core_timer_enable();

    do_mount("tmpfs", "/", "tmpfs");
    do_mkdir("/sdcard");
    do_mount("sdcard", "/sdcard", "fatfs");

    privilege_task_create(&zombie_reaper);
    privilege_task_create(&required_3_5);

    enable_irq();

    idle();
}