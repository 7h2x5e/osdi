#include <include/exc.h>
#include <include/irq.h>
#include <include/kernel_log.h>
#include <include/syscall.h>
#include <include/types.h>
#include <include/task.h>
#include <include/btree.h>
#include <include/mman.h>
#include <include/string.h>
#include <include/arm/mmu.h>
#include <include/assert.h>
#include <include/pgtable.h>
#include <include/tlbflush.h>

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

static inline void page_fault_handler(struct TrapFrame *tf)
{
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             FSC = ISS & 0b111111;
    uint64_t fault_addr;
    asm volatile("mrs %0, far_el1" : "=r"(fault_addr) :);

    KERNEL_LOG_DEBUG("Fault address 0x%x", fault_addr);

    switch (FSC) {
    case 0b000100:
        KERNEL_LOG_DEBUG("Translation fault, level 0");
        break;
    case 0b000101:
        KERNEL_LOG_DEBUG("Translation fault, level 1");
        break;
    case 0b000110:
        KERNEL_LOG_DEBUG("Translation fault, level 2");
        break;
    case 0b000111:
        KERNEL_LOG_DEBUG("Translation fault, level 3");
        break;
    case 0b001111:
        KERNEL_LOG_DEBUG("Permission fault, level 3");
        break;
    default:
        break;
    }

    /* Demand paging
     * - VMA not exist -> segfault (1)
     * - VMA exists -> alloc new page (3)
     *
     * Permission fault
     * - VMA exists
     * - page attr in VMA is:
     *     - read only -> segfault (2)
     *     - else -> fork the page (4)
     */

    task_t *cur = (task_t *) get_current();
    btree *bt = &cur->mm.mm_bt;

    // (1)
    b_key *key = bt_find_key(bt->root, fault_addr);
    if (!key) {
        KERNEL_LOG_INFO("segmentation fault");
        do_exit();
        return;
    }

    // (2)
    struct vm_area_struct *vma = (struct vm_area_struct *) (key->entry);
    pgprot_t prot = vma->vm_page_prot;
    bool WnR = tf->esr_el1 & (1ull << 6);
    // user tries to write a read-only region
    if ((pgprot_val(prot) & PD_ACCESS_PERM_2) && WnR) {
        KERNEL_LOG_INFO("segmentation fault");
        do_exit();
        return;
    }

    virtaddr_t va = ROUNDDOWN(fault_addr, PAGE_SIZE);
    page_t *pp = page_alloc();
    assert(pp);

    // (3) & (4)
    pte_t *ptep;
    if (follow_pte(&cur->mm, va, &ptep) == 0) {
        // copy on write
        memcpy((void *) PA_TO_KVA(page2pa(pp)),
               (void *) PA_TO_KVA(__pte_to_phys(*ptep)), PAGE_SIZE);
        memcpy((void *) PA_TO_KVA(page2pa(pp)),
               (void *) PA_TO_KVA(__pte_to_phys(*ptep)), PAGE_SIZE);
        unmap_page(&cur->mm, va);
    } else {
        // demand paging
        memset((void *) PA_TO_KVA(page2pa(pp)), 0, PAGE_SIZE);
        if (vma->vm_file_start != (kernaddr_t) NULL) {
            kernaddr_t addr =
                vma->vm_file_start + vma->vm_file_offset + (va - vma->vm_start);
            size_t size =
                MIN(PAGE_SIZE, vma->vm_file_len - (va - vma->vm_start));
            memcpy((void *) PA_TO_KVA(page2pa(pp)), (void *) addr, size);
        }
    }

    insert_page(&cur->mm, pp, va, prot);

    flush_tlb_all();
}

static inline void inst_abort_handler(struct TrapFrame *tf)
{
    page_fault_handler(tf);
}

static inline void data_abort_handler(struct TrapFrame *tf)
{
    page_fault_handler(tf);
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
