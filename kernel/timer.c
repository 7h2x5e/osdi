#include <include/irq.h>
#include <include/peripherals/timer.h>
#include <include/kernel_log.h>
#include <include/task.h>
#include <include/types.h>

static int system_timer_jiffies;
static int arm_timer_jiffies;
static int local_timer_jiffies;

void sys_timer_init()
{
    *SYSTEM_TIMER_COMPARE1 = *SYSTEM_TIMER_CLO + 2500000U;
    *ENABLE_IRQS_1 |= SYSTEM_TIMER_IRQ_1;
}

void sys_timer_handler()
{
    *SYSTEM_TIMER_COMPARE1 = *SYSTEM_TIMER_CLO + 2500000U;
    *SYSTEM_TIMER_CS |= SYSTEM_TIMER_CS_M1;
    KERNEL_LOG_DEBUG("system timer jiffies: %d", system_timer_jiffies++);
}

void sys_timer_disable()
{
    *DISABLE_IRQS_1 |= SYSTEM_TIMER_IRQ_1;
}

void arm_timer_init()
{
    *ARM_TIMER_CONTROL =
        ARM_TIMER_CTL_32 | ARM_TIMER_CTL_INT_EN | ARM_TIMER_CTL_EN;
    *ARM_TIMER_LOAD = 500000;
    *ENABLE_BASIC_IRQS |= ARM_TIMER_IRQ_0;
}

void arm_timer_hanler()
{
    *ARM_TIMER_IRQ_CLR = 1;
    KERNEL_LOG_DEBUG("arm timer jiffies: %d", arm_timer_jiffies++);
}

void arm_timer_disable()
{
    *DISABLE_BASIC_IRQS |= ARM_TIMER_IRQ_0;
}

void local_timer_enable()
{
    uint32_t flag = 0x30000000;  // enable timer and interrupt.
    uint32_t reload =
        (1 << 24) - 1;  // 28-bit reload value, lowest frequency = 0.14 Hz
    *LOCAL_TIMER_CONTROL_REG = flag | reload;
}

void local_timer_disable()
{
    *LOCAL_TIMER_CONTROL_REG &= !(0b11 << 28);
}

void local_timer_handler()
{
    *LOCAL_TIMER_IRQ_CLR = 0xc0000000;  // clear interrupt and reload.
    KERNEL_LOG_DEBUG("local timer jiffies: %d", local_timer_jiffies++);
}

void core_timer_enable()
{
    register uint32_t enable = 1, expired_period = EXPIRE_PERIOD;
    // enable timer
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(enable));
    // set expired time
    asm volatile("msr cntp_tval_el0, %0" : : "r"(expired_period));
    // enable timer interrupt
    *CORE0_TIMER_IRQ_CTRL |= 0x2;
}

void core_timer_disable()
{
    register uint32_t enable = 0;
    // disable timer
    asm volatile("msr cntp_ctl_el0, %0" ::"r"(enable));
    // disable timer interrupt
    *CORE0_TIMER_IRQ_CTRL &= !0x2;
}

void core_timer_handler()
{
    // set expired time
    asm volatile(
        "msr cntp_tval_el0, %[expire_period]\n\t" ::[expire_period] "r"(
            EXPIRE_PERIOD));
    task_t *current = (task_t *) get_current();
    if (current->counter > 0)
        --current->counter;
}

int64_t do_init_timers()
{
    sys_timer_init();
    arm_timer_init();
    local_timer_enable();
    core_timer_enable();
    enable_irq();
    return 0;
}