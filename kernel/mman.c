#include <include/mman.h>
#include <include/mm.h>
#include <include/arm/mmu.h>
#include <include/kernel_log.h>
#include <include/task.h>
#include <include/string.h>

#define NO_AVAILABLE_REGION ((void *) -1)

/*
 * return new regions's start address without overlap with existing regions
 * otherwise return NO_AVAILABLE_REGION
 */
static inline void *find_available_region(void *uaddr)
{
    task_t *cur_task = (task_t *) get_current();
    uint64_t *pgd, *pud, *pmd, *pte;
    uint32_t pgd_idx, pud_idx, pmd_idx, pte_idx;

    /* pgd */
    if (!cur_task->mm.pgd)
        return uaddr;
    pgd = (uint64_t *) PA_TO_KVA(cur_task->mm.pgd);

    while (1) {
        if (((uint64_t) uaddr & 0xFFFF000000000000)) {
            return NO_AVAILABLE_REGION;
        }
        pgd_idx = ((uint64_t) uaddr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
        pud_idx = ((uint64_t) uaddr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
        pmd_idx = ((uint64_t) uaddr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
        pte_idx = ((uint64_t) uaddr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

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
void *do_mmap(void *addr,
              size_t len,
              int prot,
              int flags,
              void *file_start,
              int file_offset)
{
    void *uaddr = !!addr ? addr : (void *) 0x0;

    /* align regions length */
    len = ROUNDUP(len, PAGE_SIZE);

    /* file_offset should be page aligned */
    if (file_offset != ROUNDDOWN(file_offset, PAGE_SIZE)) {
        return MAP_FAILED;
    }

    /* get page aligned addr */
    uaddr = ROUNDDOWN(uaddr, PAGE_SIZE);

    /* get regions' start address */
    if (NO_AVAILABLE_REGION == (uaddr = find_available_region(uaddr))) {
        return MAP_FAILED;
    }

    /* MAP_FIXED flag */
    if ((flags & MAP_FIXED) && (addr != NULL) && (uaddr != addr)) {
        return MAP_FAILED;
    }

    /* allocate page with protections */
    size_t old_len = len;
    void *_uaddr = uaddr, *_vaddr, *_faddr = file_start + file_offset;
    while (len > 0) {
        if (!(_vaddr = map_addr_user(_uaddr, prot))) {
            /* reclaim pages */
            // TODO: munmap()
            return MAP_FAILED;
        }

        /*
         * Region page mapping
         *
         * If the region is mapped to anonymous page frames, kernel should to do
         * 1. allocate page frames
         * 2. map memory region to page frames and set page attributes according
         * to region's protection policy
         *
         * If the region is mapped to a file, kernel should to do the above
         * things and copy the file content to the memory region
         */
        if (flags & MAP_POPULATE) {
            memcpy(_vaddr, _faddr, PAGE_SIZE);
        }

        _uaddr += PAGE_SIZE;
        _faddr += PAGE_SIZE;
        len = (len > PAGE_SIZE) ? len - PAGE_SIZE : 0;
    }
    len = old_len;  // restore len

    return uaddr;
}