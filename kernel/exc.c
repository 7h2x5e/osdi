#include <include/exc.h>
#include <include/irq.h>
#include <include/printk.h>
#include <include/syscall.h>
#include <include/types.h>

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
    uint32_t ELR_H, ELR_L;
    ELR_H = (uint32_t) (ELR >> 32);
    ELR_L = (uint32_t) ELR;
    printk("[%s]\n", entry_error_messages[TYPE]);
    printk("Exception return address 0x%h%h\n", ELR_H, ELR_L);
    printk("Exception class (EC) 0x%h\n",
           (uint32_t) ((ESR >> ESR_ELx_EC_SHIFT) & 0xFFFFFF));
    printk("Instruction specific synfrome (ISS) 0x%h\n",
           (uint32_t) (ESR & ((1 << ESR_ELx_IL_SHIFT) - 1)));
}

void sync_handler(struct TrapFrame *tf)
{
    uint32_t EC = (tf->esr_el1 >> ESR_ELx_EC_SHIFT) & 0xFFFFFF;
    switch (EC) {
    case ESR_ELx_EC_SVC64:
        svc_handler(tf);
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
