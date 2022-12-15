#ifndef _MM_H
#define _MM_H

#define KERNEL_VIRT_BASE 0xFFFF000000000000

#define PAGE_TABLE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_NUM (0x40000000 / PAGE_SIZE)
#define KERNEL_STACK_SIZE (PAGE_TABLE_SIZE << 1)  // 8KB
#define USER_VIRT_TOP 0x0000ffffffffe000ULL

#ifndef __ASSEMBLER__

#include <include/types.h>

#define KVA_TO_PA(addr) ((uint64_t) addr << 16 >> 16)
#define PA_TO_KVA(addr) ((uint64_t) addr | KERNEL_VIRT_BASE)
#define PA_TO_PFN(addr) ((uint64_t) addr >> PAGE_SHIFT)
#define PFN_TO_PA(idx) ((uint64_t) idx << PAGE_SHIFT)

enum page_flag { PAGE_USED = 1 << 0 };

typedef struct {
    uintptr_t pgd;
} mm_struct;
typedef struct {
    uint32_t flag;
    void *physical;
} page_t;

extern page_t page[PAGE_NUM];

page_t *get_free_page();
void *page_alloc_kernel();
void *page_alloc_user();
void page_free(void *virt_addr);

void mm_struct_init(mm_struct *);
void *map_addr_user(void *);
int fork_page_table(mm_struct *, const mm_struct *);

#endif
#endif