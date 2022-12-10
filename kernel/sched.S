#include <include/task.h>

.global switch_to
switch_to:
    mov x10, #THREAD_CPU_CONTEXT
    // store registers
    add x8, x0, x10
    mov x9, sp
    stp x19, x20, [x8], #16
    stp x21, x22, [x8], #16
    stp x23, x24, [x8], #16
    stp x25, x26, [x8], #16
    stp x27, x28, [x8], #16
    stp x29, x30, [x8], #16 // fp = x29, lr = x30
    str x9, [x8]

    // restore registers
    add x8, x1, x10
    ldp x19, x20, [x8], #16
    ldp x21, x22, [x8], #16
    ldp x23, x24, [x8], #16
    ldp x25, x26, [x8], #16
    ldp x27, x28, [x8], #16
    ldp x29, x30, [x8], #16
    ldr x9, [x8]
    mov sp,  x9
    msr tpidr_el1, x1
    ret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

.global update_pgd
update_pgd:
    dsb ish // ensure write has completed
    msr ttbr0_el1, x0 // switch translation based address.
    tlbi vmalle1is // invalidate all TLB entries
    dsb ish // ensure completion of TLB invalidatation
    isb // clear pipeline
    ret