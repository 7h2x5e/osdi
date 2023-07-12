#include <include/arm/mmu.h>
#include <include/mm.h>
#include <include/peripherals/base.h>
#include <include/string.h>
#include <include/types.h>
#include <include/task.h>
#include <include/mman.h>
#include <include/list.h>
#include <include/kernel_log.h>
#include <include/assert.h>
#include <include/btree.h>

#define BTREE_PAGE_NUM (1 << 13) /* 8192 */

page_t page[PAGE_NUM];
struct list_head free_page_list;
static size_t free_page_num, used_page_num;

typedef struct _btree_page {
    struct list_head head;
    union {
        b_key _x;
        b_node _y;
        btree _z;
    };
} btree_page_t;

btree_page_t b_page[BTREE_PAGE_NUM];
struct list_head free_btree_page_list;
static size_t free_btree_page_num, used_btree_page_num;

bool is_set(uint32_t target, uint32_t val)
{
    return !!(target & val);
}

void set(uint32_t *target, uint32_t val)
{
    *target |= val;
}

void unset(uint32_t *target, uint32_t val)
{
    *target &= ~val;
}

/* Page table size and content:
 * PGD: 1 page, 1 entry (point to 1 PUD)
 * PUD: 1 page, 2 entry (point to 1 PMD + 1 block)
 * PMD: 1 page, 512 entry (point to 512 PTE)
 * PTE: 1 page, 512 entry (entry of last 8 PTE point to device memory, while the
 * others point to normal memory)
 *
 * Page table layout:
 * |PGD0|PUD0|PMD0|PTE0| ... |PTE503|PTE504| ... |PTE511|
 */
void create_page_table()
{
    extern uint64_t pg_dir;
    physaddr_t *pgd, *pud, *pmd, *pte[512], page_addr = 0x0;

    pgd = (physaddr_t *) ((((uintptr_t) &pg_dir) << 16) >>
                          16);  // first 16 bits must be 0
    pud = (physaddr_t *) ((physaddr_t) pgd + PAGE_TABLE_SIZE);
    pmd = (physaddr_t *) ((physaddr_t) pud + PAGE_TABLE_SIZE);
    for (int i = 1; i <= 512; ++i) {
        pte[i] = (physaddr_t *) ((physaddr_t) pmd + PAGE_TABLE_SIZE * i);
    }

    // set up PGD
    pgd[0] = (physaddr_t) pud | PGD0_ATTR;

    // set up PUD
    pud[0] =
        (physaddr_t) pmd | PUD0_ATTR;  // 1st 1GB mapped by the 1st entry of PUD
    pud[1] = (physaddr_t) 0x40000000 |
             PUD1_ATTR;  // 2nd 1GB mapped by the 2nd entry of PUD

    // set up PMD (512 entry in 1 PMD)
    for (int i = 0; i < 512; ++i) {
        pmd[i] = (physaddr_t) pte[i] | PMD0_ATTR;
    }

    // set up PTE of normal memory
    int i;
    for (i = 0; i < 504; ++i) {
        for (int j = 0; j < 512; ++j, page_addr += PAGE_SIZE) {
            pte[i][j] = page_addr | PTE_NORMAL_ATTR;
        }
    }

    // set up PTE of device memory (0x3F000000~0x3FFFFFFF, 16 MB = 8 PTE = 4096
    // page)
    for (; i < 512; ++i) {
        for (int j = 0; j < 512; ++j, page_addr += PAGE_SIZE) {
            pte[i][j] = page_addr | PTE_DEVICE_ATTR;
        }
    }

    asm volatile(
        "msr ttbr0_el1, %0\n"  // load PGD to the buttom translation based
                               // register.
        "msr ttbr1_el1, %0\n"  // also load PGD to the upper translation based
                               // register.
        "mrs %0, sctlr_el1\n"
        "orr %0, %0, 1\n"
        "msr sctlr_el1, %0"  // enable MMU, cache remains disabled
        ::"r"(pgd));
}

void page_init()
{
    extern char _kernel_end;
    int i = 0;
    INIT_LIST_HEAD(&free_page_list);
    for (; i < PA_TO_PFN(KVA_TO_PA(&_kernel_end)); ++i) {
        // kernel stack + kernel image
        page[i].physical = (void *) ((physaddr_t) i << PAGE_SHIFT);
        page[i].flag = 0;
        page[i].pfn = i;
        set(&page[i].flag, PAGE_USED);
    }
    for (; i < PA_TO_PFN(KVA_TO_PA(MMIO_BASE)); ++i, ++free_page_num) {
        // add to free page list
        page[i].physical = (void *) ((physaddr_t) i << PAGE_SHIFT);
        page[i].flag = 0;
        page[i].pfn = i;
        list_add(&page[i].head, &free_page_list);
    }
    for (; i < PAGE_NUM; ++i) {
        // MMIO
        page[i].physical = (void *) ((physaddr_t) i << PAGE_SHIFT);
        page[i].flag = 0;
        page[i].pfn = i;
        set(&page[i].flag, PAGE_USED);
    }
    used_page_num = 0;
}

// get a page from page pool
static page_t *get_free_page()
{
    page_t *free_page = NULL;

    // find an available page from free page list
    if (list_empty(&free_page_list))
        return NULL;
    free_page = list_first_entry(&free_page_list, page_t, head);

    // sanity check
    if (is_set(free_page->flag, PAGE_USED)) {
        KERNEL_LOG_ERROR("page is in use!");
        return NULL;
    }

    // initialize the page
    uintptr_t virt_addr = (physaddr_t) free_page->physical | KERNEL_VIRT_BASE;
    memset((void *) virt_addr, 0, PAGE_SIZE);
    set(&free_page->flag, PAGE_USED);
    list_del(&free_page->head);
    free_page_num--;
    used_page_num++;

    // return pointer to page struct
    return free_page;
}

// get a page for kernel space
void *page_alloc_kernel(mm_struct *mm)
{
    page_t *page_ptr = get_free_page();
    if (!page_ptr)
        return NULL;

    physaddr_t phy_addr = (physaddr_t) page_ptr->physical;
    uintptr_t virt_addr = phy_addr | KERNEL_VIRT_BASE;

    // store a allocated page in kernel page list of mm_struct
    list_add_tail(&page_ptr->head, &mm->kernel_page_list);
    mm->kernel_page_num++;
    // KERNEL_LOG_DEBUG("allocate page for kernel space, page virt addr: %x",
    // virt_addr);

    return (void *) virt_addr;
}

// get a page for user space
void *page_alloc_user(mm_struct *mm)
{
    page_t *page_ptr = get_free_page();
    if (!page_ptr)
        return NULL;

    physaddr_t phy_addr = (physaddr_t) page_ptr->physical;
    uintptr_t virt_addr = phy_addr | KERNEL_VIRT_BASE;

    // store a allocated page in user page list of mm_struct
    list_add_tail(&page_ptr->head, &mm->user_page_list);
    mm->user_page_num++;
    // KERNEL_LOG_DEBUG("allocate page for user space, page virt addr: %x",
    // virt_addr);

    return (void *) virt_addr;
}

static inline void reclaim_page(mm_struct *mm)
{
    page_t *free_page, *tmp;
    list_for_each_entry_safe(free_page, tmp, &mm->user_page_list, head)
    {
        list_del(&free_page->head);
        list_add(&free_page->head, &free_page_list);
        unset(&free_page->flag, PAGE_USED);
        free_page_num++;
        used_page_num--;
        mm->user_page_num--;
        // KERNEL_LOG_DEBUG("free user page, virt addr: %x",
        //                  free_page->physical + KERNEL_VIRT_BASE);
    }
    list_for_each_entry_safe(free_page, tmp, &mm->kernel_page_list, head)
    {
        list_del(&free_page->head);
        list_add(&free_page->head, &free_page_list);
        unset(&free_page->flag, PAGE_USED);
        free_page_num++;
        used_page_num--;
        mm->kernel_page_num--;
        // KERNEL_LOG_DEBUG("free kernel page, virt addr: %x",
        //                  free_page->physical + KERNEL_VIRT_BASE);
    }
}

void mm_struct_init(mm_struct *mm)
{
    mm->pgd = (uintptr_t) NULL;
    mm->kernel_page_num = mm->user_page_num = 0;
    INIT_LIST_HEAD(&mm->kernel_page_list);
    INIT_LIST_HEAD(&mm->user_page_list);
    bt_init(&mm->mm_bt);
    assert(!list_empty(&mm->mm_bt.b_key_h));
}

void mm_struct_destroy(mm_struct *mm)
{
    bt_destroy(&mm->mm_bt);
    assert(list_empty(&mm->mm_bt.b_key_h));
    reclaim_page(mm);
    assert(list_empty(&mm->user_page_list));
    assert(list_empty(&mm->kernel_page_list));
    assert(mm->kernel_page_num == 0);
    assert(mm->user_page_num == 0);
    mm->pgd = (uintptr_t) NULL;
}

/* create pgd for user space of a specific process
 * return virtual address of the pgd if success, otherwise return NULL.
 */
void *create_pgd(mm_struct *mm)
{
    if (!mm->pgd) {
        // has't created pgd
        void *page_virt_addr = page_alloc_kernel(mm);
        if (!page_virt_addr)
            return NULL;
        mm->pgd = KVA_TO_PA(page_virt_addr);
    }
    return (void *) PA_TO_KVA(mm->pgd);
}

/* create pud/pmd/pte for user process
 * return virtual address of the pud/pmd/pte if success, otherwise return NULL.
 */
static void *create_pmd_pgd_pte(uint64_t *page_table,
                                uint32_t index,
                                mm_struct *mm)
{
    if (!page_table)
        return NULL;
    if (!page_table[index]) {
        void *page_virt_addr = page_alloc_kernel(mm);
        if (!page_virt_addr)
            return NULL;
        page_table[index] = KVA_TO_PA(page_virt_addr) | PD_TABLE;
    }
    return (void *) ((page_table[index] & ~0xfffULL) | KERNEL_VIRT_BASE);
}

/* create page for user process
 * return virtual address of the page if success, otherwise return NULL.
 */
static void *create_page(uint64_t *pte, uint32_t index, int prot, mm_struct *mm)
{
    if (!pte)
        return NULL;
    if (!pte[index]) {
        void *page_virt_addr = page_alloc_user(mm);
        if (!page_virt_addr)
            return NULL;
        uint64_t perm = 0;
        if (!(prot & PROT_EXEC)) {
            perm |= (1ULL << 54);
        }
        if (prot & PROT_NONE) {
            perm |= PD_ACCESS_PERM_0;
        } else {
            if (prot & PROT_READ) {
                if (prot & PROT_WRITE) {
                    perm |= PD_ACCESS_PERM_1;
                } else {
                    perm |= PD_ACCESS_PERM_3;
                }
            }
        }
        pte[index] = KVA_TO_PA(page_virt_addr) | PTE_NORMAL_ATTR | perm;
    }
    return (void *) ((pte[index] & ~0xfffULL) | KERNEL_VIRT_BASE);
}

/* Allocate new page for user process and return pages's virtual address
 * Return page address of `uaddr` if success, otherwise return 0
 */
uint64_t map_addr_user(uint64_t uaddr, int prot)
{
    /*
     * virtual user address
     * | 0.....0 | pgd index | pmd index | pud index | pte index | offset |
     *      16          9           9           9          9         12
     */
    task_t *cur_task = (task_t *) get_current();
    uint32_t pgd_idx = (uaddr & (PD_MASK << PGD_SHIFT)) >> PGD_SHIFT;
    uint32_t pud_idx = (uaddr & (PD_MASK << PUD_SHIFT)) >> PUD_SHIFT;
    uint32_t pmd_idx = (uaddr & (PD_MASK << PMD_SHIFT)) >> PMD_SHIFT;
    uint32_t pte_idx = (uaddr & (PD_MASK << PTE_SHIFT)) >> PTE_SHIFT;

    void *pgd, *pud, *pmd, *pte, *page;
    pgd = create_pgd(&cur_task->mm);
    if (!pgd)
        goto err;
    pud = create_pmd_pgd_pte(pgd, pgd_idx, &cur_task->mm);
    if (!pud)
        goto err;
    pmd = create_pmd_pgd_pte(pud, pud_idx, &cur_task->mm);
    if (!pmd)
        goto err;
    pte = create_pmd_pgd_pte(pmd, pmd_idx, &cur_task->mm);
    if (!pte)
        goto err;
    page = create_page(pte, pte_idx, prot, &cur_task->mm);
    if (!page)
        goto err;
    return (uint64_t) page;
err:
    return 0;
}

int fork_page(void *dst, const void *src)
{
    if (!src || !dst)
        return -1;
    memcpy(dst, src, PAGE_SIZE);
    return 0;
}

int fork_pte(uint64_t *dst_pt, const uint64_t *src_pt, mm_struct *dst_mm)
{
    if (!dst_pt || !src_pt)
        return -1;
    for (uint16_t idx = 0; idx < (1 << 9); idx++) {
        if (src_pt[idx]) {
            const void *next_src_pt =
                (const void *) ((src_pt[idx] & ~0xfffULL) | KERNEL_VIRT_BASE);
            void *next_dst_pt = create_page(
                dst_pt, idx, 0, dst_mm); /* set permission bits later */
            if (!next_dst_pt)
                return -1;
            dst_pt[idx] &=
                ~((3ULL << 53) | (3ULL << 6)); /* clear permission bits */
            dst_pt[idx] |=
                (src_pt[idx] & ((1ULL << 54) | (1ULL << 53) | (1 << 7) |
                                (1 << 6))); /* copy permisssion bits */
            if (-1 == fork_page(next_dst_pt, next_src_pt))
                return -1;
        }
    }
    return 0;
}

/*
 * return 0 if success, return -1 if fail.
 */
#define FORK_PAGE_TABLE(name, callback)                                      \
    int name(uint64_t *dst_pt, const uint64_t *src_pt, mm_struct *dst_mm)    \
    {                                                                        \
        if (!dst_pt || !src_pt)                                              \
            return -1;                                                       \
        for (uint16_t idx = 0; idx < (1 << 9); idx++) {                      \
            if (src_pt[idx]) {                                               \
                const void *next_src_pt =                                    \
                    (const void *) ((src_pt[idx] & ~0xfffULL) |              \
                                    KERNEL_VIRT_BASE);                       \
                void *next_dst_pt = create_pmd_pgd_pte(dst_pt, idx, dst_mm); \
                if (!next_dst_pt)                                            \
                    return -1;                                               \
                dst_pt[idx] |=                                               \
                    (src_pt[idx] & ((1ULL << 54) | (1ULL << 53) | (1 << 7) | \
                                    (1 << 6))); /* copy permisssion bits*/   \
                if (-1 == callback(next_dst_pt, next_src_pt, dst_mm))        \
                    return -1;                                               \
            }                                                                \
        }                                                                    \
        return 0;                                                            \
    }

FORK_PAGE_TABLE(fork_pmd, fork_pte)
FORK_PAGE_TABLE(fork_pud, fork_pmd)
FORK_PAGE_TABLE(fork_pgd, fork_pud)

/*
 * fork page table of user process
 * return 0 if success, return -1 if fail
 * if failed, kernel must reclaim pages outside
 */
int fork_page_table(mm_struct *dst, const mm_struct *src)
{
    if (!src || !dst)
        return -1;
    if (!src->pgd || dst->pgd)
        return -1;  // dst's pgd must be uninitialized

    uint64_t *src_pgd = (uint64_t *) PA_TO_KVA(src->pgd);
    uint64_t *dst_pgd = (uint64_t *) create_pgd(dst);

    // copy page table recursively. if failed, reclaim pages outside
    return fork_pgd(dst_pgd, src_pgd, dst);
}

void btree_page_init()
{
    INIT_LIST_HEAD(&free_btree_page_list);
    for (int i = 0; i < BTREE_PAGE_NUM; i++) {
        list_add(&b_page[i].head, &free_btree_page_list);
    }
    free_btree_page_num = BTREE_PAGE_NUM;
    used_btree_page_num = 0;
}

void *btree_page_malloc(size_t size)
{
    if (list_empty(&free_btree_page_list) || free_btree_page_num == 0)
        return NULL;
    btree_page_t *p =
        list_first_entry(&free_btree_page_list, btree_page_t, head);
    list_del(&p->head);
    free_btree_page_num--;
    used_btree_page_num++;
    return (void *) p;
}

void btree_page_free(void *ptr)
{
    if (!ptr)
        return;
    btree_page_t *p = (btree_page_t *) ptr;
    list_add(&p->head, &free_btree_page_list);
    free_btree_page_num++;
    used_btree_page_num--;
}

/* return 0 if success, return -1 if fail */
int fork_btree(mm_struct *mm_dst, const mm_struct *mm_src)
{
    uint32_t err;
    b_key *tmp;
    btree *dst = &mm_dst->mm_bt;
    const btree *src = &mm_src->mm_bt;
    list_for_each_entry(tmp, &src->b_key_h, b_key_h)
    {
        if (is_minimum(tmp) || is_maximum(tmp))
            continue;

        assert((tmp->start + PAGE_SIZE) == tmp->end);

        KERNEL_LOG_DEBUG("Fork region [%x, %x)", tmp->start, tmp->end);

        if ((err = bt_insert_range(&dst->root, tmp->start, tmp->end, PAGE_SIZE,
                                   tmp->entry, tmp->f_addr, tmp->f_size,
                                   tmp->prot))) {
            KERNEL_LOG_ERROR("Cannot fork region [%x, %x), err=%d", tmp->start,
                             tmp->end, err);
            return -1;
        }
    }
    return 0;
}
