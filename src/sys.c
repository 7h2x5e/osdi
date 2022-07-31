#include "sys.h"
#include "irq.h"
#include "peripherals/irq.h"
#include "peripherals/timer.h"
#include "peripherals/uart.h"
#include "types.h"

extern void enable_irq();
extern void disable_irq();
void sys_toggle_irq(uint8_t);
void sys_timestamp();
void gpu_irq_handler();

const void *sys_call_table[] = {[SYS_TOGGLE_IRQ_NUMBER] = sys_toggle_irq,
                                [SYS_TIMESTAMP_NUMBER] = sys_timestamp};

const char *entry_error_messages[] = {
    "SYNC_INVALID_EL1t",   "IRQ_INVALID_EL1t",
    "FIQ_INVALID_EL1t",    "ERROR_INVALID_EL1T",

    "SYNC_INVALID_EL1h",   "IRQ_INVALID_EL1h",
    "FIQ_INVALID_EL1h",    "ERROR_INVALID_EL1h",

    "SYNC_INVALID_EL0_64", "IRQ_INVALID_EL0_64",
    "FIQ_INVALID_EL0_64",  "ERROR_INVALID_EL0_64",

    "SYNC_INVALID_EL0_32", "IRQ_INVALID_EL0_32",
    "FIQ_INVALID_EL0_32",  "ERROR_INVALID_EL0_32",

    "SYNC_ERROR",          "SYSCALL_ERROR"};

#define DMUP_EXCEPTION_REGISTER(elr, ec, iss)                           \
    ({                                                                  \
        uart_printf("Exception return address 0x%h\n", elr);            \
        uart_printf("Exception class (EC) 0x%h\n", ec);                 \
        uart_printf("Instruction specific synfrome (ISS) 0x%h\n", iss); \
    })

void show_invalid_entry_message(uint64_t type, uint64_t esr, uint64_t elr)
{
    uint32_t ec, iss;
    ec = esr >> 26;
    iss = (esr << 40) >> 40;
    uart_printf("[%s]\n", entry_error_messages[type]);
    DMUP_EXCEPTION_REGISTER(elr, ec, iss);
}

void sys_timestamp(uint64_t *freq, uint64_t *pct)
{
    register uint64_t f, t;
    // Get the current counter frequency & counter
    asm volatile(
        "mrs %[CNTFRQ], cntfrq_el0\n"
        "mrs %[CNTPCT], cntpct_el0"
        : [CNTFRQ] "=r"(f), [CNTPCT] "=r"(t)
        :);
    *freq = f;
    *pct = t;
}

void sys_toggle_irq(uint8_t enable)
{
    if (!!enable) {
        // Even initializers of sys timer, arm timer, local timer can execute at
        // el0 I move them to irq handler along with core timer initializer.
#if _SYS_TIMER == 1
        sys_timer_init();
#endif
#if _ARM_TIMER == 1
        arm_timer_init();
#endif
#if _LOCAL_TIMER == 1
        local_timer_enable();
#endif
#if _CORE_TIMER == 1
        core_timer_enable();
#endif
        enable_irq();  // enable timer
    } else {
        disable_irq();  // disable timer
#if _SYS_TIMER == 1
        sys_timer_disable();
#endif
#if _ARM_TIMER == 1
        arm_timer_disable();
#endif
#if _LOCAL_TIMER == 1
        local_timer_disable();
#endif
#if _CORE_TIMER == 1
        core_timer_disable();
#endif
    }
}

void irq_handler(unsigned long esr, unsigned long elr)
{
    uint32_t basic_irq, local_irq;

#if _ARM_TIMER == 1
    basic_irq = *IRQ_BASIC_PENDING;
    if (ARM_TIMER_IRQ_0 & basic_irq) {
        arm_timer_hanler();
    }
#endif

    do {
        local_irq = *CORE0_IRQ_SRC;
        if (local_irq) {
            switch (1U << __builtin_ctz(local_irq)) {
#if (_LOCAL_TIMER == 1 || _CORE_TIMER == 1)
            case CNTPNSIRQ:
                core_timer_handler();
                break;
            case LOCAL_TIMER_IRQ:
                local_timer_handler();
                break;
#endif
            case GPU_IRQ:
                gpu_irq_handler();
                break;
            default:
            }
        }
    } while (local_irq);
}

void gpu_irq_handler()
{
    uint32_t gpu_irq1, gpu_irq2;
    do {
        gpu_irq1 = *IRQ_PENDING_1;
        gpu_irq2 = *IRQ_PENDING_2;
        if (gpu_irq1) {
            switch (1U << __builtin_ctz(gpu_irq1)) {
#if _SYS_TIMER == 1
            case SYSTEM_TIMER_IRQ_1:
                sys_timer_handler();
                break;
#endif
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