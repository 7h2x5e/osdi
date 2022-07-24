#include "irq.h"
#include "peripherals/timer.h"
#include "sys.h"
#include "types.h"

void toggle_irq(uint8_t enable)
{
    /* wrapper of sys_toggle_irq */
    asm volatile("mov x8," xstr(SYS_TOGGLE_IRQ_NUMBER) "\n"
                                                   "svc #0");
}