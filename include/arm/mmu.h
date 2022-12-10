#ifndef _MMU_H
#define _MMU_H

/* Translation Control Register (TCR) */
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

/* Memory attribute indirection register (MAIR) */
#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

/* Page descriptor */
#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_PAGE 0b11
#define PD_ACCESS (1 << 10)
#define PD_ACCESS_PERM_0 (0b00 << 6)  // EL0: NA, EL1: RW
#define PD_ACCESS_PERM_1 (0b01 << 6)  // EL0: RW, EL1: RW
#define PD_ACCESS_PERM_2 (0b10 << 6)  // EL0: NA, EL1: RO
#define PD_ACCESS_PERM_3 (0b11 << 6)  // EL0: RO, EL1: RO
#define PD_MASK 0x1ffULL

#define PGD0_ATTR PD_TABLE
#define PUD0_ATTR PD_TABLE
#define PUD1_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define PMD0_ATTR PD_TABLE
#define PTE_NORMAL_ATTR (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE)
#define PTE_DEVICE_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE)

#define PGD_SHIFT 39
#define PUD_SHIFT 30
#define PMD_SHIFT 21
#define PTE_SHIFT 12

#endif
