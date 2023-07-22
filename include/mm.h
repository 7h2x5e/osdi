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
#include <include/list.h>
#include <include/btree.h>

#define KVA_TO_PA(addr) ((uint64_t) (addr) << 16 >> 16)
#define PA_TO_KVA(addr) ((uint64_t) (addr) | KERNEL_VIRT_BASE)
#define PA_TO_PFN(addr) ((uint64_t) (addr) >> PAGE_SHIFT)
#define PFN_TO_PA(idx) ((uint64_t) (idx) << PAGE_SHIFT)

enum page_flag { PAGE_USED = 1 << 0 };

typedef struct {
    uintptr_t pgd;                     /* page global directory */
    struct list_head kernel_page_list; /* list of allocated kernel pages */
    uint32_t kernel_page_num;          /* number of kernel pages */
    btree mm_bt;                       /* use B-Tree to manage user pages */
} mm_struct;

typedef struct {
    uint32_t flag;
    int pfn;
    void *physical;
    struct list_head head; /* for user page, record processes using the page;
                              for kernel page, attach to page list of
                              `mm_struct` and set `refcnt` to 1 */
    uint32_t refcnt;
} page_t;

extern page_t page[PAGE_NUM];
extern struct list_head free_page_list;
extern size_t free_page_num, used_page_num;

bool is_set(uint32_t, uint32_t);
void set(uint32_t *, uint32_t);
void unset(uint32_t *, uint32_t);
void *create_pgd(mm_struct *);
void *page_alloc_kernel(mm_struct *);
void *page_alloc_user(mm_struct *, uint64_t);
void mm_struct_init(mm_struct *);
void mm_struct_destroy(mm_struct *);
uint64_t map_addr_user(uint64_t, int prot);
int fork_page(void *dst, const void *src);
int fork_page_table(mm_struct *, const mm_struct *);
void *btree_page_malloc(size_t size);
void btree_page_free(void *ptr);
int fork_btree(mm_struct *, const mm_struct *);

#endif
#endif