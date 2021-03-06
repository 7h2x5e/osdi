#include "fb.h"
#include "irq.h"
#include "peripherals/uart.h"
#include "shell.h"

void main()
{
    uart_init(115200);
    uart_flush();
    fb_init();
    fb_showpicture();
    shell();
    return;
}