#ifndef SYSREGS_H
#define SYSREGS_H

// SCTLR_EL1, System Control Register (EL1)
#define SCTLR_RESERVED (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_EE_LITTLE_ENDIAN (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN (0 << 24)
#define SCTLR_I_CACHE_DISABLED (0 << 12)
#define SCTLR_D_CACHE_DISABLED (0 << 2)
#define SCTLR_MMU_DISABLED (0 << 0)
#define SCTLR_MMU_ENABLED (1 << 0)

#define SCTLR_VALUE_MMU_DISABLED                                        \
    (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | \
     SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

// Hypervisor Configuration Register (EL2)
#define HCR_EL2_RW (1 << 31)
#define HCR_EL2_VALUE (HCR_EL2_RW)

// Saved Program Status Register
#define SPSR_EL0t (0b0000)
#define SPSR_EL1t (0b0100)
#define SPSR_EL1h (0b0101)
#define SPSR_EL2t (0b1000)
#define SPSR_EL2h (0b1001)
#define SPSR_EL1_MASK_NONE (0 << 6)
#define SPSR_EL2_MASK_ALL (7 << 6)
#define SPSR_EL1_VALUE (SPSR_EL1_MASK_NONE | SPSR_EL0t)
#define SPSR_EL2_VALUE (SPSR_EL2_MASK_ALL | SPSR_EL1h)

// Stack pointer
#define SP_EL0_VALUE 0x60000
#define SP_EL1_VALUE 0x80000

// Architectural Feature Access Control Register
#define CPACR_EL1_FPEN (0b11 << 20)
#define CPACR_EL1_VALUE (CPACR_EL1_FPEN)

// ESR_EL1, Exception Syndrome Register (EL1)
#define ESR_ELx_EC_SHIFT 26
#define ESR_ELx_EC_SVC64 0x15

#endif
