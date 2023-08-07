#include <include/mman.h>
#include <include/mm.h>
#include <include/arm/mmu.h>
#include <include/kernel_log.h>
#include <include/task.h>
#include <include/string.h>
#include <include/btree.h>
#include <include/error.h>

void *do_mmap(void *addr,
              size_t len,
              mmap_prot_t prot,
              mmap_flags_t flags,
              void *file_start,
              off_t file_offset)
{
    task_t *cur = (task_t *) get_current();
    btree *bt = &cur->mm.mm_bt;

    if (flags & MAP_FIXED) {
        if ((virtaddr_t) addr & ((1ull << PAGE_SHIFT) - 1)) {
            return MAP_FAILED;
        }
    }

    uint64_t start =
        (addr == NULL) ? bt->min : (uint64_t) ROUNDDOWN(addr, PAGE_SIZE);
    if (!bt_find_empty_area(bt->root, start, bt->max, ROUNDUP(len, PAGE_SIZE),
                            (uint64_t *) &addr)) {
        return MAP_FAILED;
    }

    pgprot_t attr = __pgprot(PD_ACCESS_PERM_0);
    if (prot & PROT_READ) {
        if (prot & PROT_WRITE) {
            pgprot_val(attr) |= PD_ACCESS_PERM_1;
        } else {
            pgprot_val(attr) |= PD_ACCESS_PERM_3;
        }
    }
    if (!(prot & PROT_EXEC)) {
        pgprot_val(attr) |= PD_ACCESS_EXEC;
    }

    virtaddr_t first = (virtaddr_t) addr;
    virtaddr_t last = (virtaddr_t) addr + len;

    struct vm_area_struct *vma = vma_alloc();
    if (vma == NULL) {
        return MAP_FAILED;
    }

    vma->vm_start = first;
    vma->vm_end = last;
    vma->vm_mm = &cur->mm;
    vma->vm_page_prot = attr;
    vma->vm_file_start = (kernaddr_t) file_start;
    vma->vm_file_offset = file_offset;
    vma->vm_file_len = len;

    if (bt_insert_range(&bt->root, (uint64_t) addr,
                        (uint64_t) addr + ROUNDUP(len, PAGE_SIZE),
                        (void *) vma) != 0) {
        vma_free(vma);
        return MAP_FAILED;
    }

    KERNEL_LOG_INFO("do_mmap: address 0x%x, length %d", addr, len);

    return addr;
}
