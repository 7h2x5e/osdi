#ifndef PERIPHERAL_IRQ_H
#define PERIPHERAL_IRQ_H

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

#endif
