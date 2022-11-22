#include <include/arm/mmu.h>
#include <include/mm.h>
#include <include/types.h>

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