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
        uint64_t u_addr = ROUNDDOWN(fault_addr, PAGE_SIZE), p_addr;

        task_t *cur = (task_t *) get_current();
        btree *bt = &cur->mm.mm_bt;
        b_key *key = bt_find_key(bt->root, u_addr);
        page_info *p_pf;

        if (!key || key->start != u_addr) {
            KERNEL_LOG_INFO("Could not find region in btree");
            goto page_fault_end;
        }
        p_pf = &key->pf;

        /*
         * Demand paging, kernel should:
         * 1. allocate page frames
         * 2. map memory region to page frames and set page attributes
         * according to region's protection policy
         */
        if (!(p_addr = map_addr_user(u_addr, p_pf->prot))) {
            KERNEL_LOG_INFO("Cannot allocate page for user space, addr=%x",
                            u_addr);
            do_munmap(u_addr, PAGE_SIZE);
            goto page_fault_end;
        }

        /* If the region maps to a file, kernel should: copy the file content to
         * the memory region. Otherwise, zero fill the region */
        if (p_pf->f_size > 0) {
            memcpy((void *) p_addr, (void *) p_pf->f_addr, PAGE_SIZE);
        } else {
            memset((void *) p_addr, 0, PAGE_SIZE);
        }

        KERNEL_LOG_DEBUG("Successfully load page, region [%x, %x)", u_addr,
                         u_addr + PAGE_SIZE);

        return;
    } else if (status_code == 0b001111) { /* Permission fault, level 3. */
        /* align page address */
        uint64_t u_addr = ROUNDDOWN(fault_addr, PAGE_SIZE), p_addr;

        task_t *cur = (task_t *) get_current();
        mm_struct *mm = &cur->mm;
        btree *bt = &mm->mm_bt;
        b_key *key = bt_find_key(bt->root, u_addr);
        page_info *p_pf = &key->pf;

        /* If the corresponding region is read-only, then the segmentation
         * fault is generated because user trying to write a read-only
         * region.
         * If the corresponding region is read-wrtie, then it’s a copy-on-write
         * fault. Kernel should allocate one page frame, copy the data, and
         * modify the table’s entry to be correct permission. */
        if (!(p_pf->prot & PROT_WRITE)) {
            KERNEL_LOG_INFO("Segmentation fault addr=%x", u_addr);
            goto page_fault_end;
        }

        assert(p_pf->p_page != NULL);
        page_t *p = (page_t *) p_pf->p_page;
        assert(p->refcnt > 0);

        /* unbind `page_info` with original page */
        p_pf->p_page = NULL;
        list_del(&p_pf->head);

        /* set pte entry to zero to make `create_page` allocate new page */
        uint64_t *pgd, *pud, *pmd, *pte;
        uint16_t pgd_idx, pud_idx, pmd_idx, pte_idx;
        pgd_idx = (u_addr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
        pud_idx = (u_addr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
        pmd_idx = (u_addr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
        pte_idx = (u_addr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;
        pgd = (uint64_t *) PA_TO_KVA(mm->pgd);
        pud = (uint64_t *) PA_TO_KVA(pgd[pgd_idx] & ~0xfffULL);
        pmd = (uint64_t *) PA_TO_KVA(pud[pud_idx] & ~0xfffULL);
        pte = (uint64_t *) PA_TO_KVA(pmd[pmd_idx] & ~0xfffULL);
        pte[pte_idx] = 0;

        /* allocate a new page for the region */
        if (!(p_addr = map_addr_user(u_addr, p_pf->prot))) {
            KERNEL_LOG_INFO("Cannot allocate page for user space, addr=%x",
                            u_addr);
            do_munmap(u_addr, PAGE_SIZE);
            goto page_fault_end;
        }

        assert((pte[pte_idx] & ~0xfffULL) != 0);
        assert(p_pf->p_page != NULL);

        KERNEL_LOG_DEBUG("CoW, successfully create page");

        /* fork original page */
        if (-1 ==
            fork_page((void *) p_addr, (void *) PA_TO_KVA(PFN_TO_PA(p->pfn)))) {
            goto page_fault_end;
        }

        KERNEL_LOG_DEBUG("CoW, successfully fork page (0x%x -> 0x%x)",
                         PA_TO_KVA(PFN_TO_PA(p->pfn)), p_addr);

        /* decrease reference count of original page */
        if (--p->refcnt == 0) {
            assert(list_empty(&p->head));
            list_add(&p->head, &free_page_list);
            unset(&p->flag, PAGE_USED);
            free_page_num++;
            used_page_num--;

            KERNEL_LOG_TRACE("return user page to free list, used page=%d",
                             used_page_num);
        }

        return;
    }

page_fault_end:
    KERNEL_LOG_INFO("Kill process...");
    do_exit();
}

static inline void inst_abort_handler(struct TrapFrame *tf)
{
    uint64_t far_el1;
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             IFSC = ISS & 0b111111;
    asm volatile("mrs %0, far_el1" : "=r"(far_el1) :);
    KERNEL_LOG_DEBUG("Instruction Abort! IFSC 0x%x, page fault address 0x%x",
                     IFSC, far_el1);
    page_fault_handler(IFSC, far_el1);
}

static inline void data_abort_handler(struct TrapFrame *tf)
{
    uint64_t far_el1;
    uint32_t ISS = (tf->esr_el1) & ((1 << ESR_ELx_IL_SHIFT) - 1),
             DFSC = ISS & 0b111111;
    asm volatile("mrs %0, far_el1" : "=r"(far_el1) :);
    KERNEL_LOG_DEBUG("Data Abort! DFSC 0x%x, page fault address 0x%x", DFSC,
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
