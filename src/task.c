#include "task.h"
#include "peripherals/uart.h"
#include "sched.h"

void task1()
{
    while (1) {
        uart_printf("1...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        reschedule();
    }
    __builtin_unreachable();
}

void task2()
{
    while (1) {
        uart_printf("2...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
        reschedule();
    }
    __builtin_unreachable();
}

void utask3()
{
    while (1) {
        uart_printf("3...\n");
        for (int i = 0; i < (1 << 26); ++i)
            asm("nop");
    }
}

void task3()
{
    do_exec(&utask3);
    __builtin_unreachable();
}