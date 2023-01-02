#include <include/exc.h>
#include <include/irq.h>
#include <include/printk.h>
#include <include/syscall.h>
#include <include/types.h>
#include <include/task.h>

const static char *entry_error_messages[] = {
    "SYNC_INVALID_EL1t",   "IRQ_INVALID_EL1t",
    "FIQ_INVALID_EL1t",    "ERROR_INVALID_EL1T",

    "SYNC_INVALID_EL1h",   "IRQ_INVALID_EL1h",
    "FIQ_INVALID_EL1h",    "ERROR_INVALID_EL1h",

    "SYNC_INVALID_EL0_64", "IRQ_INVALID_EL0_64",
    "FIQ_INVALID_EL0_64",  "ERROR_INVALID_EL0_64",

    "SYNC_INVALID_EL0_32", "IRQ_INVALID_EL0_32",
    "FIQ_INVALID_EL0_32",  "ERROR_INVALID_EL0_32",

    "SYNC_ERROR",          "SYSCALL_ERROR"};

void show_invalid_entry_message(struct TrapFrame *tf)
{
    uint64_t TYPE = tf->type, ESR = tf->esr_el1, ELR = tf->elr_el1;
    printk("[%s]\n", entry_error_messages[TYPE]);
    printk("Exception return address 0x%h\n", ELR);
    printk("Exception class (EC) 0x%h\n", (ESR >> ESR_ELx_EC_SHIFT) & 0xFFFFFF);
    printk("Instruction specific synfrome (ISS) 0x%h\n",
           ESR & ((1 << ESR_ELx_IL_SHIFT) - 1));
}

static inline void page_fault_handler()
{
    uint64_t far_el1;
    asm volatile("mrs %0, far_el1" : "=r"(far_el1) :);
    printk("[PID %d] Page fault address 0x%h, kill process\n", do_get_taskid(),
           far_el1);
    do_exit();
}

static inline void inst_abort_handler(struct TrapFrame *tf)
{
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             IFSC = ISS & 0b111111;
    printk("[PID %d] Instruction Abort! IFSC 0x%h\n", do_get_taskid(), IFSC);
    page_fault_handler();
}

static inline void data_abort_handler(struct TrapFrame *tf)
{
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             DFSC = ISS & 0b111111;
    printk("[PID %d] Data Abort! DFSC 0x%h\n", do_get_taskid(), DFSC);
    page_fault_handler();
}

void sync_handler(struct TrapFrame *tf)
{
    uint32_t EC = (tf->esr_el1 >> ESR_ELx_EC_SHIFT) & 0xFFFFFF;
    switch (EC) {
    case ESR_ELx_EC_SVC64:
        svc_handler(tf);
        break;
    case ESR_ELx_EC_INST_ABORT_LOW:
        inst_abort_handler(tf);
        break;
    case ESR_ELx_EC_DATA_ABORT_LOW:
        data_abort_handler(tf);
        break;
    default:
        printk("Exceptions with an unknown reason\n");
        show_invalid_entry_message(tf);
    }
}

void svc_handler(struct TrapFrame *tf)
{
    syscall_handler(tf);
}
