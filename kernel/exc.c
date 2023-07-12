#include <include/exc.h>
#include <include/irq.h>
#include <include/kernel_log.h>
#include <include/syscall.h>
#include <include/types.h>
#include <include/task.h>
#include <include/btree.h>
#include <include/mman.h>
#include <include/string.h>

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
    KERNEL_LOG_INFO("[%s]", entry_error_messages[TYPE]);
    KERNEL_LOG_INFO("Exception return address 0x%x", ELR);
    KERNEL_LOG_INFO("Exception class (EC) 0x%x",
                    (ESR >> ESR_ELx_EC_SHIFT) & 0xFFFFFF);
    KERNEL_LOG_INFO("Instruction specific synfrome (ISS) 0x%x",
                    ESR & ((1 << ESR_ELx_IL_SHIFT) - 1));
}

static inline void page_fault_handler(uint32_t status_code, uint64_t fault_addr)
{
    if ((status_code == 0b000100)    /* Translation fault, level 0. */
        || (status_code == 0b000101) /* Translation fault, level 1. */
        || (status_code == 0b000110) /* Translation fault, level 2. */
        || (status_code == 0b000111) /* Translation fault, level 3. */
    ) {
        /* align page address */
        uint64_t v_addr = ROUNDDOWN(fault_addr, PAGE_SIZE), p_addr;

        task_t *cur = (task_t *) get_current();
        btree *bt = &cur->mm.mm_bt;
        b_key *key = bt_find_key(bt->root, v_addr);

        if (!key || key->start != v_addr) {
            KERNEL_LOG_DEBUG("Could not find region in btree");
            goto page_fault_end;
        }

        /*
         * Demand paging, kernel should:
         * 1. allocate page frames
         * 2. map memory region to page frames and set page attributes
         * according to region's protection policy
         */
        if (!(p_addr = map_addr_user(v_addr, key->prot))) {
            KERNEL_LOG_DEBUG("Cannot allocate page for user space, addr=%x",
                             v_addr);
            do_munmap(v_addr, PAGE_SIZE);
            goto page_fault_end;
        }

        /* append page virtual address to key */
        key->entry = (void *) p_addr;

        /* If the region maps to a file, kernel should: copy the
         * file content to the memory region */
        if (key->f_size > 0) {
            memcpy((void *) p_addr, (void *) key->f_addr, PAGE_SIZE);
        }

        KERNEL_LOG_DEBUG("Successfully load page");
        return;
    }

page_fault_end:
    KERNEL_LOG_ERROR("Kill process...");
    do_exit();
}

static inline void inst_abort_handler(struct TrapFrame *tf)
{
    uint64_t far_el1;
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             IFSC = ISS & 0b111111;
    asm volatile("mrs %0, far_el1" : "=r"(far_el1) :);
    KERNEL_LOG_ERROR("Instruction Abort! IFSC 0x%x, page fault address 0x%x",
                     IFSC, far_el1);
    page_fault_handler(IFSC, far_el1);
}

static inline void data_abort_handler(struct TrapFrame *tf)
{
    uint64_t far_el1;
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             DFSC = ISS & 0b111111;
    asm volatile("mrs %0, far_el1" : "=r"(far_el1) :);
    KERNEL_LOG_ERROR("Data Abort! DFSC 0x%x, page fault address 0x%x", DFSC,
                     far_el1);
    page_fault_handler(DFSC, far_el1);
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
        KERNEL_LOG_INFO("Exceptions with an unknown reason");
        show_invalid_entry_message(tf);
    }
}

void svc_handler(struct TrapFrame *tf)
{
    syscall_handler(tf);
}
