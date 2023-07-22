#include <include/mman.h>
#include <include/mm.h>
#include <include/arm/mmu.h>
#include <include/kernel_log.h>
#include <include/task.h>
#include <include/string.h>
#include <include/btree.h>
#include <include/error.h>

#if 0
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
#endif


/*
 * return regions' start address if succeeded or MAP_FAILED if failed. all user
 * pages containing a part of the indicated range will result in returning
 * MAP_FAILED
 */
uint64_t do_mmap(uint64_t addr,
                 uint64_t len,
                 uint32_t prot,
                 uint32_t flags,
                 uint64_t file_start,
                 uint64_t file_offset)
{
    uint32_t err;

    /* retrieve B-Tree from mm_struct */
    task_t *cur = (task_t *) get_current();
    btree *bt = &cur->mm.mm_bt;

    /* align page address */
    uint64_t v_addr = ROUNDDOWN(addr, PAGE_SIZE);

    /* align region length */
    len = ROUNDUP(len, PAGE_SIZE);

    /* get regions' start address */
    if (!(bt->max > v_addr && bt->max - v_addr >= len)) {
        KERNEL_LOG_INFO("no available memory area");
        return MAP_FAILED;
    } else if (!bt_find_empty_area(bt->root, v_addr, bt->max, len, &v_addr)) {
        KERNEL_LOG_INFO("no available memory area");
        return MAP_FAILED;
    }

    if ((flags & MAP_FIXED) && (v_addr != addr)) {
        return MAP_FAILED;
    }

    /* align file_offset */
    if (file_offset != ROUNDDOWN(file_offset, PAGE_SIZE)) {
        return MAP_FAILED;
    }

    KERNEL_LOG_DEBUG("mmap: addr = 0x%x (0x%x), len = %d", v_addr, addr, len);

    /* allocate page with protections */
    size_t old_len = len;
    uint64_t tmp_addr = v_addr, f_addr = file_start + file_offset;
    while (len > 0) {
        /* demand paging. store page info and load it when page fault happens.
         * load 1 page from f_addr in page fault handler */
        page_info pg_info = {.f_addr = f_addr,
                             .f_size = PAGE_SIZE,
                             .prot = prot,
                             .p_page = NULL};
        if (file_start == 0) {
            pg_info.f_addr = pg_info.f_size = 0;
        }

        err = bt_insert_range(&bt->root, tmp_addr, tmp_addr + PAGE_SIZE,
                              PAGE_SIZE, &pg_info);

        if (err) {
            KERNEL_LOG_INFO(
                "Cannot insert region in btree, addr = %x, err = %d", tmp_addr,
                err);
            goto _do_mmap_err;
        }

        tmp_addr += PAGE_SIZE;
        f_addr += PAGE_SIZE;
        len = (len > PAGE_SIZE) ? len - PAGE_SIZE : 0;
    }
    len = old_len;  // restore len

    return v_addr;

_do_mmap_err:
    for (uint64_t free_addr = v_addr; free_addr < tmp_addr;
         free_addr += PAGE_SIZE) {
        do_munmap(free_addr, PAGE_SIZE);
    }
    return MAP_FAILED;
}

/* On success, munmap() returns 0.  On failure, it returns -1 */
int32_t do_munmap(uint64_t addr, uint64_t length)
{
    length =
        ROUNDUP(length, PAGE_SIZE);  // length must be a multiple of PAGE_SIZE

    /* retrieve B-Tree from mm_struct */
    task_t *cur = (task_t *) get_current();
    btree *bt = &cur->mm.mm_bt;

    uint64_t _start;
    for (uint64_t tmp_addr = addr; length > 0;
         length -= PAGE_SIZE, tmp_addr += PAGE_SIZE) {
        if (bt_find_empty_area(bt->root, tmp_addr, tmp_addr + PAGE_SIZE,
                               PAGE_SIZE, &_start)) {
            KERNEL_LOG_DEBUG("Cannot find indicated range [%x, %x) in btree",
                             tmp_addr, tmp_addr + PAGE_SIZE);
            continue;
        }

        KERNEL_LOG_DEBUG("Unmap page, virtual addr [%x, %x)", tmp_addr,
                         tmp_addr + PAGE_SIZE);
        //@todo unbind key and page
        //@todo remove indicated range from btree
        //@todo invalidate page entry
    }
    return 0;
}