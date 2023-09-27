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
#define VMA_NUM 4096

#ifndef __ASSEMBLER__

#include <include/types.h>
#include <include/list.h>
#include <include/pgtable-types.h>
#include <include/btree.h>

#define KVA_TO_PA(addr) ((uint64_t) (addr) << 16 >> 16)
#define PA_TO_KVA(addr) ((uint64_t) (addr) | KERNEL_VIRT_BASE)
#define PA_TO_PFN(addr) ((uint64_t) (addr) >> PAGE_SHIFT)
#define PFN_TO_PA(idx) ((uint64_t) (idx) << PAGE_SHIFT)

enum page_flag { PAGE_USED = 1 << 0 };

typedef struct {
    pgd_t *pgd;
    btree mm_bt;
} mm_struct;

typedef struct {
    struct list_head buddy_list;
    uint32_t refcnt;
    uint8_t order;
} page_t;

struct vm_area_struct {
    virtaddr_t vm_start;
    virtaddr_t vm_end;
    mm_struct *vm_mm;
    pgprot_t vm_page_prot;
    kernaddr_t vm_file_start;
    off_t vm_file_offset;
    size_t vm_file_len;
    struct list_head alloc_link;
};

typedef struct {
    union {
        b_key _x;
        b_node _y;
        btree _z;
    };
    struct list_head head;
} btree_node_t;

void mem_init();
void buddy_init();
page_t *buddy_alloc(uint8_t);
void buddy_free(page_t *);
void page_init();
page_t *page_alloc();
void unmap_page(mm_struct *mm, virtaddr_t addr);
physaddr_t page2pa(page_t *);
page_t *pa2page(physaddr_t);
int32_t follow_pte(mm_struct *mm, virtaddr_t address, pte_t **ptepp);
int32_t insert_page(mm_struct *, page_t *, virtaddr_t, pgprot_t);
void mm_init(mm_struct *);
void mm_destroy(mm_struct *);
void copy_mm(mm_struct *dst, const mm_struct *src);
void btree_node_init();
void *btree_node_malloc(size_t size);
void btree_node_free(void *ptr);
void vma_init();
struct vm_area_struct *vma_alloc();
void vma_free(struct vm_area_struct *);

#endif
#endif