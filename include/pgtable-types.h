#ifndef _PGTABLE_TYPES_H
#define _PGTABLE_TYPES_H

#include <include/types.h>

typedef uint64_t pteval_t;
typedef uint64_t pmdval_t;
typedef uint64_t pudval_t;
typedef uint64_t pgdval_t;

typedef struct {
    pteval_t pte;
} pte_t;
#define pte_val(x) ((x).pte)
#define pte_none(pte) (!pte_val(pte))
#define __pte(x) ((pte_t){(x)})

typedef struct {
    pmdval_t pmd;
} pmd_t;
#define pmd_val(x) ((x).pmd)
#define __pmd(x) ((pmd_t){(x)})
#define pmd_none(pmd) (!pmd_val(pmd))

typedef struct {
    pudval_t pud;
} pud_t;
#define pud_val(x) ((x).pud)
#define __pud(x) ((pud_t){(x)})
#define pud_none(pud) (!pud_val(pud))

typedef struct {
    pgdval_t pgd;
} pgd_t;
#define pgd_val(x) ((x).pgd)
#define __pgd(x) ((pgd_t){(x)})
#define pgd_none(pgd) (!pgd_val(pgd))

typedef struct {
    pteval_t pgprot;
} pgprot_t;
#define pgprot_val(x) ((x).pgprot)
#define __pgprot(x) ((pgprot_t){(x)})

#endif