#include <include/assert.h>
#include <include/printk.h>
#include <include/irq.h>
#include <include/peripherals/uart.h>

const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void _panic(const char *file, int line, const char *fmt, ...)
{
    if (panicstr)
        goto dead;
    panicstr = fmt;

    // Be extra sure that the machine is in as reasonable state
    disable_irq();
    uart_set_mode(UART_POLLING_MODE);

    printk("kernel panic at %s:%d\n", file, line);
    printk(fmt);
    printk("\n");

dead:
    while (1)
        ;
}

/* like panic, but don't */
void _warn(const char *file, int line, const char *fmt, ...)
{
    printk("kernel warn at %s:%d\n", file, line);
    printk(fmt);
    printk("\n");
}
