.section ".text.boot"

#include "mm.h"
#include "asm/sysregs.h"

.global _start
_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, el2_init
proc_hang:  
    // cpu id > 0, stop
    wfe
    b       proc_hang

el2_init:
	ldr	    x0, =SCTLR_VALUE_MMU_DISABLED
	msr	    sctlr_el1, x0		

    ldr	    x0, =HCR_EL2_VALUE
	msr	    hcr_el2, x0

    // Do not cause trap when using Advanced SIMD and floating point register on EL0 & EL1
    ldr     x1, =CPACR_EL1_VALUE
    msr     CPACR_EL1, x1

    // set stack pointer for el1
    ldr     x0, =SP_EL1_VALUE
    msr     sp_el1, x0

    // switch to el1
    adr     x0, el1_init
    msr     elr_el2, x0
    ldr     x0, =SPSR_EL2_VALUE
    msr     spsr_el2, x0
    
    eret

el1_init:
    // clear bss
	adr     x0, __bss_start
	adr     x1, __bss_end
	sub     x1, x1, x0
	bl      memzero

    bl      main
    
    // for failsafe
    b       proc_hang

    // load exception_table to VBAR_EL1
    ldr     x0, =exception_table
    msr     VBAR_EL1, x0

    // set stack pointer for el0
    ldr     x0, =SP_EL0_VALUE
    msr     sp_el0, x0

    // switch to el0
    adr     x0, el0_init
    msr     elr_el1, x0
    ldr     x0, =SPSR_EL1_VALUE
    msr     spsr_el1, x0

    eret

el0_init:
    // jump to C code, should not return
    bl      main
    // for failsafe, halt this core too
    b       proc_hang
