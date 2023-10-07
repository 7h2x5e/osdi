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
#include <include/pgtable.h>
#include <include/assert.h>
#include <include/error.h>
#include <include/tlbflush.h>
#include <include/buddy.h>
#include <include/slab.h>

static void page_free(page_t *pp);
static void page_decref(page_t *);
static int32_t __pud_alloc(mm_struct *, pgd_t *, virtaddr_t);
static int32_t __pmd_alloc(mm_struct *, pud_t *, virtaddr_t);
static int32_t __pte_alloc(mm_struct *, pmd_t *, virtaddr_t);
static pte_t *walk_to_pte(mm_struct *, virtaddr_t);
static void free_pgtables(mm_struct *);
static void mm_alloc_pgd(mm_struct *mm);
static void pgtable_test();

static kernaddr_t nextfree;  // virtual address of next byte of free memory
static page_t *pages;
struct buddy_system buddy_system;

static inline pgd_t *pgd_alloc()
{
    page_t *pp = page_alloc();
    pgd_t *pgd = (pgd_t *) PA_TO_KVA(page2pa(pp));
    return pgd;
}

static inline pud_t *pud_alloc(mm_struct *mm, pgd_t *pgd, virtaddr_t address)
{
    return (pgd_none(*pgd) && __pud_alloc(mm, pgd, address))
               ? NULL
               : pud_offset(pgd, address);
}

static inline pmd_t *pmd_alloc(mm_struct *mm, pud_t *pud, virtaddr_t address)
{
    return (pud_none(*pud) && __pmd_alloc(mm, pud, address))
               ? NULL
               : pmd_offset(pud, address);
}

static inline pte_t *pte_alloc(mm_struct *mm, pmd_t *pmd, virtaddr_t address)
{
    return (pmd_none(*pmd) && __pte_alloc(mm, pmd, address))
               ? NULL
               : pte_offset(pmd, address);
}

static kernaddr_t boot_alloc(uint32_t n)
{
    kernaddr_t result;

    if (!nextfree) {
        extern char _kernel_end[];
        nextfree = ROUNDUP((kernaddr_t) _kernel_end, PAGE_SIZE);
    }

    if (n == 0) {
        return nextfree;
    } else {
        result = nextfree;
        nextfree += ROUNDUP(n, PAGE_SIZE);
    }

    return result;
}

void mem_init()
{
    KERNEL_LOG_INFO("nextfree=0x%x", boot_alloc(0));

    pages = (page_t *) boot_alloc(sizeof(page_t) * PAGE_NUM);
    buddy_init();

    pgtable_test();
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
    extern uint64_t pg_dir[];
    physaddr_t *pgd, *pud, *pmd, *pte[512], page_addr = 0x0;

    pgd = (physaddr_t *) ((((uintptr_t) pg_dir) << 16) >>
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

static inline void add_page_to_free_list(page_t *pp,
                                         struct buddy_system *buddy_system,
                                         uint8_t order)
{
    list_add(&pp->buddy_list, &buddy_system->free_area[order].free_list);
    buddy_system->free_area[order].nr_free++;
}

static inline void del_page_from_free_list(page_t *pp,
                                           struct buddy_system *buddy_system,
                                           uint8_t order)
{
    list_del_init(&pp->buddy_list);
    buddy_system->free_area[order].nr_free--;
}

static inline page_t *buddy_find(page_t *pp, uint8_t order)
{
    return pa2page(page2pa(pp) ^ (1ull << (order + PAGE_SHIFT)));
}

static inline uint8_t buddy_order(uint64_t size)
{
    assert(size >= PAGE_SIZE);
    return 63 - __builtin_clzll(size >> PAGE_SHIFT);
}

void buddy_init()
{
    for (uint8_t order = 0; order < MAX_ORDER; order++) {
        INIT_LIST_HEAD(&buddy_system.free_area[order].free_list);
    }

    for (physaddr_t addr = 0; addr < KVA_TO_PA(MMIO_BASE); addr += PAGE_SIZE) {
        page_t *pp = pa2page(addr);
        INIT_LIST_HEAD(&pp->buddy_list);
    }

    physaddr_t next_buddy_addr = KVA_TO_PA(nextfree);
    physaddr_t physical_mmio_addr = KVA_TO_PA(MMIO_BASE);
    while (next_buddy_addr < physical_mmio_addr) {
        uint8_t order = MIN(MAX_ORDER - 1,
                            buddy_order(physical_mmio_addr - next_buddy_addr));
        page_t *pp = pa2page(next_buddy_addr);
        pp->order = order;
        add_page_to_free_list(pp, &buddy_system, order);
        next_buddy_addr += 1ULL << (PAGE_SHIFT + order);
    }
}

page_t *buddy_alloc(uint8_t order)
{
    uint8_t target = order;
    while (target < MAX_ORDER && buddy_system.free_area[target].nr_free == 0)
        target++;
    if (target == MAX_ORDER) {
        return NULL;
    }

    while (target > order) {
        page_t *pp1 =
                   list_first_entry(&buddy_system.free_area[target].free_list,
                                    page_t, buddy_list),
               *pp2 = buddy_find(pp1, target - 1);
        del_page_from_free_list(pp1, &buddy_system, target);
        add_page_to_free_list(pp1, &buddy_system, target - 1);
        add_page_to_free_list(pp2, &buddy_system, target - 1);
        pp1->order = pp2->order = target - 1;
        target--;
    }

    page_t *free_page = list_first_entry(
        &buddy_system.free_area[order].free_list, page_t, buddy_list);
    del_page_from_free_list(free_page, &buddy_system, order);
    return free_page;
}

void buddy_free(page_t *pp)
{
    virtaddr_t start = PA_TO_KVA(boot_alloc(0));

    add_page_to_free_list(pp, &buddy_system, pp->order);

    while (start < MMIO_BASE) {
        page_t *pp = pa2page(KVA_TO_PA(start));

        if (!list_empty(&pp->buddy_list)) {
            while (pp->order < MAX_ORDER - 1 && PAGE_SIZE < MMIO_BASE - start) {
                page_t *b_pp = buddy_find(pp, pp->order);

                if (list_empty(&b_pp->buddy_list) || (pp->order != b_pp->order))
                    break;

                del_page_from_free_list(pp, &buddy_system, pp->order);
                del_page_from_free_list(b_pp, &buddy_system, b_pp->order);
                pp->order++;
                add_page_to_free_list(pp, &buddy_system, pp->order);
            }
        }

        start += 1ULL << (PAGE_SHIFT + pp->order);
    }
}

page_t *page_alloc()
{
    page_t *free_page = buddy_alloc(0);
    if (!free_page)
        return NULL;
    free_page->refcnt = 0;
    free_page->page_slab = NULL;
    memset((void *) PA_TO_KVA(page2pa(free_page)), 0, PAGE_SIZE);
    return free_page;
}

static void page_free(page_t *pp)
{
    if (pp->refcnt != 0)
        panic("page_free error");
    buddy_free(pp);
}

static void page_decref(page_t *pp)
{
    if (--pp->refcnt == 0) {
        page_free(pp);
    }
}

static void mm_alloc_pgd(mm_struct *mm)
{
    if (!mm->pgd)
        mm->pgd = pgd_alloc(mm);
}

static void free_pte(pte_t *pte_base)
{
    for (size_t idx = 0; idx < PAGE_SIZE / sizeof(pte_t); ++idx) {
        pte_t *pte = pte_base + idx;
        if (!pte_none(*pte)) {
            page_t *pp = pa2page(__pte_to_phys(*pte));
            page_decref(pp);
            *pte = __pte(0);
        }
    }

    page_t *pp = pa2page(KVA_TO_PA((kernaddr_t) pte_base));
    page_decref(pp);
}

static void free_pmd(pmd_t *pmd_base)
{
    for (size_t idx = 0; idx < PAGE_SIZE / sizeof(pmd_t); ++idx) {
        pmd_t *pmd = pmd_base + idx;
        if (!pmd_none(*pmd)) {
            free_pte(pmd_pgtable(pmd));
            *pmd = __pmd(0);
        }
    }

    page_t *pp = pa2page(KVA_TO_PA((kernaddr_t) pmd_base));
    page_decref(pp);
}

static void free_pud(pud_t *pud_base)
{
    for (size_t idx = 0; idx < PAGE_SIZE / sizeof(pud_t); ++idx) {
        pud_t *pud = pud_base + idx;
        if (!pud_none(*pud)) {
            free_pmd(pud_pgtable(pud));
            *pud = __pud(0);
        }
    }

    page_t *pp = pa2page(KVA_TO_PA((kernaddr_t) pud_base));
    page_decref(pp);
}

static void free_pgd(mm_struct *mm)
{
    for (size_t idx = 0; idx < PAGE_SIZE / sizeof(pgd_t); ++idx) {
        pgd_t *pgd = mm->pgd + idx;
        if (!pgd_none(*pgd)) {
            free_pud(pgd_pgtable(pgd));
            *pgd = __pgd(0);
        }
    }

    page_t *pp = pa2page(KVA_TO_PA((kernaddr_t) mm->pgd));
    page_decref(pp);
    mm->pgd = NULL;
}

static void free_pgtables(mm_struct *mm)
{
    free_pgd(mm);
}

physaddr_t page2pa(page_t *pp)
{
    if (pp < pages) {
        panic("page2pa called with invalid page");
    }
    return (physaddr_t) ((uintptr_t) (pp - pages) << PAGE_SHIFT);
}

page_t *pa2page(physaddr_t pa)
{
    if (PA_TO_PFN(pa) >= PAGE_NUM) {
        panic("pa2page called with invalid pa");
    }
    return &pages[PA_TO_PFN(pa)];
}

void mm_init(mm_struct *mm)
{
    mm_alloc_pgd(mm);
    bt_init(&mm->mm_bt);
}

void mm_destroy(mm_struct *mm)
{
    free_pgtables(mm);
    bt_destroy(&mm->mm_bt);
}

static int32_t __pud_alloc(mm_struct *mm, pgd_t *pgd, virtaddr_t address)
{
    page_t *pp = page_alloc();
    pud_t *new = (pud_t *) page2pa(pp);
    if (!new) {
        return -E_NO_MEM;
    }

    pgdval_t pgdval = PGD_TYPE_TABLE;
    *pgd = __pgd((pgdval_t) new | pgdval);
    pp->refcnt++;

    return 0;
}

static int32_t __pmd_alloc(mm_struct *mm, pud_t *pud, virtaddr_t address)
{
    page_t *pp = page_alloc();
    pud_t *new = (pud_t *) page2pa(pp);
    if (!new) {
        return -E_NO_MEM;
    }

    pudval_t pudval = PUD_TYPE_TABLE;
    *pud = __pud((pudval_t) new | pudval);
    pp->refcnt++;

    return 0;
}

static int32_t __pte_alloc(mm_struct *mm, pmd_t *pmd, virtaddr_t address)
{
    page_t *pp = page_alloc();
    pmd_t *new = (pmd_t *) page2pa(pp);
    if (!new) {
        return -E_NO_MEM;
    }

    pmdval_t pmdval = PMD_TYPE_TABLE;
    *pmd = __pmd((pmdval_t) new | pmdval);
    pp->refcnt++;

    return 0;
}

static pte_t *walk_to_pte(mm_struct *mm, virtaddr_t addr)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    if (!mm->pgd)
        return NULL;
    pgd = pgd_offset(mm, addr);
    pud = pud_alloc(mm, pgd, addr);
    if (!pud)
        return NULL;
    pmd = pmd_alloc(mm, pud, addr);
    if (!pmd)
        return NULL;
    pte = pte_alloc(mm, pmd, addr);

    return pte;
}

int32_t insert_page(mm_struct *mm, page_t *pp, virtaddr_t addr, pgprot_t prot)
{
    pte_t *pte = walk_to_pte(mm, addr);
    if (!pte)
        return -E_NO_MEM;
    if (!pte_none(*pte))
        return -E_BUSY;

    *pte = __pte((pteval_t) page2pa(pp) | pgprot_val(prot) | PTE_NORMAL_ATTR);
    pp->refcnt++;

    return 0;
}

int32_t follow_pte(mm_struct *mm, virtaddr_t address, pte_t **ptepp)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *ptep;

    if (!mm->pgd)
        goto out;

    pgd = pgd_offset(mm, address);
    if (pgd_none(*pgd)) {
        goto out;
    }

    pud = pud_offset(pgd, address);
    if (pud_none(*pud)) {
        goto out;
    }

    pmd = pmd_offset(pud, address);
    if (pmd_none(*pmd)) {
        goto out;
    }

    ptep = pte_offset(pmd, address);
    if (pte_none(*ptep)) {
        goto out;
    }

    *ptepp = ptep;

    return 0;
out:
    return -E_INVAL;
}

void unmap_page(mm_struct *mm, virtaddr_t addr)
{
    pte_t *pte;
    int32_t ret = follow_pte(mm, addr, &pte);
    if (ret)
        return;

    physaddr_t pa = __pte_to_phys(*pte);
    page_t *pp = pa2page(pa);

    page_decref(pp);
    *pte = __pte(0);
}

void copy_mm(mm_struct *dst, const mm_struct *src)
{
    b_key *key;
    bt_for_each(&src->mm_bt, key)
    {
        if (is_minimum(key) || is_maximum(key))
            continue;

        struct vm_area_struct *new_vma = NULL;
        if (key->entry) {
            struct vm_area_struct *vma = key->entry;
            new_vma = vma_alloc();
            if (new_vma == NULL) {
                panic("vma_alloc error");
            }

            new_vma->vm_start = vma->vm_start;
            new_vma->vm_end = vma->vm_end;
            new_vma->vm_mm = dst;
            new_vma->vm_page_prot = vma->vm_page_prot;

            pte_t *ptep;
            for (virtaddr_t va = key->start; va < key->end; va += PAGE_SIZE) {
                if (follow_pte((mm_struct *) src, va, &ptep) == 0) {
                    *ptep = __pte(pte_val(*ptep) | PD_ACCESS_PERM_3);  // RO
                    page_t *pp = pa2page(__pte_to_phys(*ptep));
                    insert_page(dst, pp, va,
                                __pgprot(pgprot_val(vma->vm_page_prot) |
                                         PD_ACCESS_PERM_3));  // RO
                }
            }
        }

        bt_insert_range(&dst->mm_bt.root, key->start, key->end,
                        (void *) new_vma);
    }
}

struct vm_area_struct *vma_alloc()
{
    struct vm_area_struct *vma =
        (struct vm_area_struct *) kmalloc(sizeof(struct vm_area_struct));
    return vma;
}

void vma_free(struct vm_area_struct *vma)
{
    if (!vma)
        return;
    kfree(vma);
}

static void pgtable_test()
{
    mm_struct mm_instance;
    mm_struct *mm = &mm_instance;
    mm->pgd = pgd_alloc();
    assert(mm->pgd);

    virtaddr_t va = 0x123456789000;
    pgd_t *pgd = pgd_offset(mm, va);
    assert((pgd - mm->pgd) == 36);
    KERNEL_LOG_INFO("pgd_offset() succeeded!");

    assert(pgd_val(*(mm->pgd)) == 0);
    pud_t *pud = pud_alloc(mm, pgd, va);
    assert((pgd_val(*pgd) & ~PTE_ADDR_MASK) == PD_TABLE);
    KERNEL_LOG_INFO("pud_alloc() succeeded!");

    assert(pud_val(*pud) == 0);
    pmd_t *pmd = pmd_alloc(mm, pud, va);
    assert((pud_val(*pud) & ~PTE_ADDR_MASK) == PD_TABLE);
    KERNEL_LOG_INFO("pmd_alloc() succeeded!");

    assert(pmd_val(*pmd) == 0);
    pte_t *pte = pte_alloc(mm, pmd, va);
    assert((pmd_val(*pmd) & ~PTE_ADDR_MASK) == PD_TABLE);
    KERNEL_LOG_INFO("pte_alloc() succeeded!");

    assert(walk_to_pte(mm, va) == pte);
    KERNEL_LOG_INFO("walk_to_pte() succeeded!");

    page_t *pp = page_alloc();
    assert(pp);
    assert(pp->refcnt == 0);
    memset((void *) PA_TO_KVA(page2pa(pp)), 0x55, PAGE_SIZE);
    pgprot_t prot = __pgprot(PTE_NORMAL_ATTR);
    assert(insert_page(mm, pp, va, prot) == 0);
    assert(pp->refcnt == 1);

    __asm__ volatile("msr TTBR0_EL1, %0" ::"r"(mm->pgd));
    flush_tlb_all();

    assert(*(uint64_t *) va == 0x5555555555555555);
    KERNEL_LOG_INFO("change TTBR0_EL1 succeeded!");

    memset((void *) va, 0xaa, PAGE_SIZE);
    assert(*(uint64_t *) PA_TO_KVA(page2pa(pp)) == 0xaaaaaaaaaaaaaaaa);
    KERNEL_LOG_INFO("write virtual address succeeded!");

    virtaddr_t va2 = 0x987654321000;
    assert(insert_page(mm, pp, va2, prot) == 0);
    assert(pp->refcnt == 2);
    flush_tlb_all();
    assert(*(uint64_t *) va2 == 0xaaaaaaaaaaaaaaaa);
    KERNEL_LOG_INFO("page sharing succeeded!");

    unmap_page(mm, va2);
    assert(pp->refcnt == 1);
    assert(follow_pte(mm, va2, &pte) == -E_INVAL);
    assert(follow_pte(mm, va, &pte) == 0);
    KERNEL_LOG_INFO("unmap_page() succeeded!");

    free_pgtables(mm);
    assert(pa2page(KVA_TO_PA((kernaddr_t) mm->pgd))->refcnt == 0);
    assert(pp->refcnt == 0);
    assert(follow_pte(mm, va, &pte) == -E_INVAL);
    KERNEL_LOG_INFO("free_pgtables() succeeded!");
}