#include <include/mman.h>
#include <include/mm.h>
#include <include/arm/mmu.h>
#include <include/kernel_log.h>
#include <include/task.h>
#include <include/string.h>

#define NO_AVAILABLE_REGION ((uint64_t) -1)

/*
 * return new regions's start address without overlap with existing regions
 * otherwise return NO_AVAILABLE_REGION
 */
static inline uint64_t find_available_region(uint64_t uaddr)
{
    task_t *cur_task = (task_t *) get_current();
    uint64_t *pgd, *pud, *pmd, *pte;
    uint32_t pgd_idx, pud_idx, pmd_idx, pte_idx;

    /* pgd */
    if (!cur_task->mm.pgd)
        return uaddr;
    pgd = (uint64_t *) PA_TO_KVA(cur_task->mm.pgd);

    while (1) {
        if ((uaddr & 0xFFFF000000000000)) {
            return NO_AVAILABLE_REGION;
        }
        pgd_idx = (uaddr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
        pud_idx = (uaddr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
        pmd_idx = (uaddr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
        pte_idx = (uaddr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

        /* pud */
        if (!pgd[pgd_idx])
            break;
        pud = (uint64_t *) PA_TO_KVA(pgd[pgd_idx] & ~0xfffULL);

        /* pmd */
        if (!pud[pud_idx])
            break;
        pmd = (uint64_t *) PA_TO_KVA(pud[pud_idx] & ~0xfffULL);

        /* pte */
        if (!pmd[pmd_idx])
            break;
        pte = (uint64_t *) PA_TO_KVA(pmd[pmd_idx] & ~0xfffULL);

        /* page */
        if (!pte[pte_idx])
            break;

        uaddr += PAGE_SIZE;
    }
    return uaddr;
}


/*
 * return regions' start address if succeeded or MAP_FAILED if failed
 */
uint64_t do_mmap(uint64_t addr,
                 uint64_t len,
                 uint32_t prot,
                 uint32_t flags,
                 uint64_t file_start,
                 uint64_t file_offset)
{
    /* align page address */
    uint64_t v_addr = ROUNDDOWN(addr, PAGE_SIZE);

    /* align region length */
    len = ROUNDUP(len, PAGE_SIZE);

    /* align file_offset*/
    if (file_offset != ROUNDDOWN(file_offset, PAGE_SIZE)) {
        return MAP_FAILED;
    }

    /* get regions' start address */
    if (NO_AVAILABLE_REGION == (v_addr = find_available_region(v_addr))) {
        return MAP_FAILED;
    }

    if ((flags & MAP_FIXED) && (v_addr != addr)) {
        return MAP_FAILED;
    }

    /* allocate page with protections */
    size_t old_len = len;
    uint64_t tmp_addr = v_addr, p_addr, f_addr = file_start + file_offset;
    while (len > 0) {
        if (!(p_addr = map_addr_user(tmp_addr, prot))) {
            // TODO: munmap()
            return MAP_FAILED;
        }

        /*
         * Region page mapping
         *
         * If the region maps to anonymous page frames, kernel should:
         * 1. allocate page frames
         * 2. map memory region to page frames and set page attributes according
         * to region's protection policy
         *
         * If the region maps to a file, kernel should:
         * 1. copy the file content to the memory region
         */
        if (flags & MAP_POPULATE) {
            memcpy((void *) p_addr, (void *) f_addr, PAGE_SIZE);
        }

        tmp_addr += PAGE_SIZE;
        f_addr += PAGE_SIZE;
        len = (len > PAGE_SIZE) ? len - PAGE_SIZE : 0;
    }
    len = old_len;  // restore len

    return v_addr;
}