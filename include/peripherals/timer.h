#ifndef TIMER_H
#define TIMER_H

#include "peripherals/base.h"

#define IRQ_BASIC_PENDING ((volatile unsigned int *) (MMIO_BASE + 0x0000B200))
#define IRQ_PENDING_1 ((volatile unsigned int *) (MMIO_BASE + 0x0000B204))
#define IRQ_PENDING_2 ((volatile unsigned int *) (MMIO_BASE + 0x0000B208))
#define FIQ_CONTROL ((volatile unsigned int *) (MMIO_BASE + 0x0000B20C))
#define ENABLE_IRQS_1 ((volatile unsigned int *) (MMIO_BASE + 0x0000B210))
#define ENABLE_IRQS_2 ((volatile unsigned int *) (MMIO_BASE + 0x0000B214))
#define ENABLE_BASIC_IRQS ((volatile unsigned int *) (MMIO_BASE + 0x0000B218))
#define DISABLE_IRQS_1 ((volatile unsigned int *) (MMIO_BASE + 0x0000B21C))
#define DISABLE_IRQS_2 ((volatile unsigned int *) (MMIO_BASE + 0x0000B220))
#define DISABLE_BASIC_IRQS ((volatile unsigned int *) (MMIO_BASE + 0x0000B224))

// System timer, not emulated by qemu
#define SYSTEM_TIMER_BASE (MMIO_BASE + 0x00003000)
#define SYSTEM_TIMER_CS ((volatile unsigned int *) (SYSTEM_TIMER_BASE))
#define SYSTEM_TIMER_CLO \
    ((volatile unsigned int *) (SYSTEM_TIMER_BASE + 0x00000004))
#define SYSTEM_TIMER_CHI \
    ((volatile unsigned int *) (SYSTEM_TIMER_BASE + 0x00000008))
#define SYSTEM_TIMER_COMPARE0 \
    ((volatile unsigned int *) (SYSTEM_TIMER_BASE + 0x0000000C))
#define SYSTEM_TIMER_COMPARE1 \
    ((volatile unsigned int *) (SYSTEM_TIMER_BASE + 0x00000010))
#define SYSTEM_TIMER_COMPARE2 \
    ((volatile unsigned int *) (SYSTEM_TIMER_BASE + 0x00000014))
#define SYSTEM_TIMER_COMPARE3 \
    ((volatile unsigned int *) (SYSTEM_TIMER_BASE + 0x00000018))
#define SYSTEM_TIMER_IRQ_0 (1 << 0)
#define SYSTEM_TIMER_IRQ_1 (1 << 1)
#define SYSTEM_TIMER_IRQ_2 (1 << 2)
#define SYSTEM_TIMER_IRQ_3 (1 << 3)
#define SYSTEM_TIMER_CS_M0 (1 << 0)
#define SYSTEM_TIMER_CS_M1 (1 << 1)
#define SYSTEM_TIMER_CS_M2 (1 << 2)
#define SYSTEM_TIMER_CS_M3 (1 << 3)

// ARM side timer, not emulated by qemu
#define ARM_TIMER_BASE (MMIO_BASE + 0x0000B000)
#define ARM_TIMER_LOAD ((volatile unsigned int *) (ARM_TIMER_BASE + 0x00000400))
#define ARM_TIMER_VALUE \
    ((volatile unsigned int *) (ARM_TIMER_BASE + 0x00000404))  // Read only
#define ARM_TIMER_CONTROL \
    ((volatile unsigned int *) (ARM_TIMER_BASE + 0x00000408))
#define ARM_TIMER_IRQ_CLR \
    ((volatile unsigned int *) (ARM_TIMER_BASE + 0x0000040C))  // Write only
#define ARM_TIMER_RAW_IRQ \
    ((volatile unsigned int *) (ARM_TIMER_BASE + 0x00000410))  // Read only
#define ARM_TIMER_MASK_IRQ \
    ((volatile unsigned int *) (ARM_TIMER_BASE + 0x00000414))  // Read only
#define ARM_TIMER_RELOAD \
    ((volatile unsigned int *) (ARM_TIMER_BASE + 0x00000418))
#define ARM_TIMER_CTL_16 (0 << 1)
#define ARM_TIMER_CTL_32 (1 << 1)
#define ARM_TIMER_CTL_INT_EN (1 << 5)
#define ARM_TIMER_CTL_EN (1 << 7)
#define ARM_TIMER_IRQ_0 (1 << 0)

// ARM local timer & ARM core timer, emulated by qemu
#define LOCAL_TIMER_CONTROL_REG \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000034))
#define LOCAL_TIMER_IRQ_CLR \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000038))
#define CORE0_TIMER_IRQ_CTRL \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000040))
#define CORE1_TIMER_IRQ_CTRL \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000044))
#define CORE2_TIMER_IRQ_CTRL \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000048))
#define CORE3_TIMER_IRQ_CTRL \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x0000004C))
#define CORE0_IRQ_SRC \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000060))
#define CORE1_IRQ_SRC \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000064))
#define CORE2_IRQ_SRC \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x00000068))
#define CORE3_IRQ_SRC \
    ((volatile unsigned int *) (LOCAL_PERIPHERAL_BASE + 0x0000006C))
#define CNTPSIRQ (1 << 0)
#define CNTPNSIRQ (1 << 1)
#define CNTHPIRQ (1 << 2)
#define CNTVIRQ (1 << 3)
#define MAILBOX0_IRQ (1 << 4)
#define MAILBOX1_IRQ (1 << 5)
#define MAILBOX2_IRQ (1 << 6)
#define MAILBOX3_IRQ (1 << 7)
#define GPU_IRQ (1 << 8)
#define PMU_IRQ (1 << 9)
#define AXI_IRQ (1 << 10)
#define LOCAL_TIMER_IRQ (1 << 11)

#define EXPIRE_PERIOD 0xfffffff

// Enable bit
#define _SYS_TIMER 1
#define _ARM_TIMER 0
#define _LOCAL_TIMER 1
#define _CORE_TIMER 1

void sys_timer_init();
void sys_timer_handler();
void sys_timer_disable();
void arm_timer_init();
void arm_timer_hanler();
void arm_timer_disable();
void local_timer_enable();
void local_timer_disable();
void local_timer_handler();
void core_timer_enable();
void core_timer_disable();
void core_timer_handler();

#endif
