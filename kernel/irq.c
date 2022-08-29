#include <include/exc.h>
#include <include/irq.h>
#include <include/peripherals/irq.h>
#include <include/peripherals/timer.h>
#include <include/peripherals/uart.h>
#include <include/printk.h>

void enable_irq()
{
    asm("msr daifclr, #2");
}

void disable_irq()
{
    asm("msr daifset, #2");
}

void gpu_irq_handler()
{
    uint32_t gpu_irq1, gpu_irq2;
    do {
        gpu_irq1 = *IRQ_PENDING_1;
        gpu_irq2 = *IRQ_PENDING_2;
        if (gpu_irq1) {
            switch (1U << __builtin_ctz(gpu_irq1)) {
            case SYSTEM_TIMER_IRQ_1:
                sys_timer_handler();
                break;
            default:
            }
        }
        if (gpu_irq2) {
            switch (1U << __builtin_ctz(gpu_irq2)) {
            case UART_IRQ:
                uart_handler();
                break;
            default:
            }
        }
    } while (gpu_irq1 || gpu_irq2);
}

void irq_handler(struct TrapFrame *tf)
{
    uint32_t basic_irq, local_irq;

    basic_irq = *IRQ_BASIC_PENDING;
    if (ARM_TIMER_IRQ_0 & basic_irq) {
        arm_timer_hanler();
    }

    do {
        local_irq = *CORE0_IRQ_SRC;
        if (local_irq) {
            switch (1U << __builtin_ctz(local_irq)) {
            case CNTPNSIRQ:
                core_timer_handler();
                break;
            case LOCAL_TIMER_IRQ:
                local_timer_handler();
                break;
            case GPU_IRQ:
                gpu_irq_handler();
                break;
            default:
            }
        }
    } while (local_irq);
}