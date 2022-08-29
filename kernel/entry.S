#include <include/entry.h>

.macro	kernel_entry, el, type
sub	sp, sp, #S_FRAME_SIZE
stp	x0, x1, [sp, #16 * 0]
stp	x2, x3, [sp, #16 * 1]
stp	x4, x5, [sp, #16 * 2]
stp	x6, x7, [sp, #16 * 3]
stp	x8, x9, [sp, #16 * 4]
stp	x10, x11, [sp, #16 * 5]
stp	x12, x13, [sp, #16 * 6]
stp	x14, x15, [sp, #16 * 7]
stp	x16, x17, [sp, #16 * 8]
stp	x18, x19, [sp, #16 * 9]
stp	x20, x21, [sp, #16 * 10]
stp	x22, x23, [sp, #16 * 11]
stp	x24, x25, [sp, #16 * 12]
stp	x26, x27, [sp, #16 * 13]
stp	x28, x29, [sp, #16 * 14]

.if \el == 0
mrs x21, sp_el0
.else
add x21, sp, #S_FRAME_SIZE // x21 = sp + #S_FRAME_SIZE
.endif /* \el == 0 */

mov x22, #\type
mrs	x23, esr_el1
mrs	x24, elr_el1
mrs x25, spsr_el1
stp x30, x21, [sp, #16 * 15]
stp x22, x23, [sp, #16 * 16]
stp x24, x25, [sp, #16 * 17]
.endm

.macro	kernel_exit, el
.if	\el == 0
bl  reschedule
.endif /* \el == 0 */

ldp	x24, x25, [sp, #16 * 17]
ldp	x22, x23, [sp, #16 * 16]
ldp	x30, x21, [sp, #16 * 15]
msr	elr_el1, x24
msr	spsr_el1, x25

.if	\el == 0
msr	sp_el0, x21
.endif /* \el == 0 */

ldp	x0, x1, [sp, #16 * 0]
ldp	x2, x3, [sp, #16 * 1]
ldp	x4, x5, [sp, #16 * 2]
ldp	x6, x7, [sp, #16 * 3]
ldp	x8, x9, [sp, #16 * 4]
ldp	x10, x11, [sp, #16 * 5]
ldp	x12, x13, [sp, #16 * 6]
ldp	x14, x15, [sp, #16 * 7]
ldp	x16, x17, [sp, #16 * 8]
ldp	x18, x19, [sp, #16 * 9]
ldp	x20, x21, [sp, #16 * 10]
ldp	x22, x23, [sp, #16 * 11]
ldp	x24, x25, [sp, #16 * 12]
ldp	x26, x27, [sp, #16 * 13]
ldp	x28, x29, [sp, #16 * 14]
add	sp, sp, #S_FRAME_SIZE	
eret
.endm

.macro handle_invalid_entry el, type
kernel_entry \el, \type
mov x0, sp
b show_invalid_entry_message
kernel_exit \el
.endm

.macro	ventry	label
.align	7
b	\label
.endm

/*
 * Exception vectors.
 */
.section ".text.exception_table"
.align	11
.globl exception_table 
exception_table:
	ventry	sync_invalid_el1t			// Synchronous EL1t
	ventry	irq_invalid_el1t			// IRQ EL1t
	ventry	fiq_invalid_el1t			// FIQ EL1t
	ventry	error_invalid_el1t			// Error EL1t

    ventry	sync_invalid_el1h        	// Synchronous EL1h
    ventry	el1_irq						// IRQ EL1h
	ventry	fiq_invalid_el1h			// FIQ EL1h
	ventry	error_invalid_el1h			// Error EL1h

	ventry	el0_sync     				// Synchronous 64-bit EL0
	ventry	el0_irq				        // IRQ 64-bit EL0
	ventry	fiq_invalid_el0_64			// FIQ 64-bit EL0
    ventry	error_invalid_el0_64		// Error 64-bit EL0

	ventry	sync_invalid_el0_32			// Synchronous 32-bit EL0
	ventry	irq_invalid_el0_32			// IRQ 32-bit EL0
	ventry	fiq_invalid_el0_32			// FIQ 32-bit EL0
	ventry	error_invalid_el0_32		// Error 32-bit EL0

sync_invalid_el1t:
	handle_invalid_entry  1, SYNC_INVALID_EL1t

irq_invalid_el1t:
	handle_invalid_entry  1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
	handle_invalid_entry  1, FIQ_INVALID_EL1t

error_invalid_el1t:
	handle_invalid_entry  1, ERROR_INVALID_EL1t

sync_invalid_el1h:
	handle_invalid_entry  1, SYNC_INVALID_EL1h

fiq_invalid_el1h:
	handle_invalid_entry  1 FIQ_INVALID_EL1h

error_invalid_el1h:
	handle_invalid_entry  1, ERROR_INVALID_EL1h

sync_invalid_el0_64:
	handle_invalid_entry  0, SYNC_INVALID_EL0_64

irq_invalid_el0_64:
	handle_invalid_entry  0, IRQ_INVALID_EL0_64

fiq_invalid_el0_64:
	handle_invalid_entry  0, FIQ_INVALID_EL0_64

error_invalid_el0_64:
	handle_invalid_entry  0, ERROR_INVALID_EL0_64

sync_invalid_el0_32:
	handle_invalid_entry  0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
	handle_invalid_entry  0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
	handle_invalid_entry  0, FIQ_INVALID_EL0_32

error_invalid_el0_32:
	handle_invalid_entry  0, ERROR_INVALID_EL0_32

el1_irq:
    kernel_entry 1, UNDEFINED
	mov x0, sp
    bl  irq_handler
    kernel_exit 1

el0_irq:
    kernel_entry 0, UNDEFINED
	mov x0, sp
    bl  irq_handler
    kernel_exit 0

el0_sync:
    kernel_entry 0, UNDEFINED
	mov x0, sp
    bl  sync_handler
    kernel_exit 0