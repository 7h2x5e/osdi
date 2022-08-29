#include <include/peripherals/mbox.h>
#include <include/peripherals/uart.h>

volatile unsigned int __attribute__((aligned(16))) mbox[36];

/*
 * Returns 0 on failure, non-zero on success.
 */
int mbox_call(unsigned char ch)
{
    /* Write to a mailbox*/
    register unsigned int r =
        (unsigned int) (((unsigned long) &mbox) & (~0xF)) | (ch & 0xF);
    // read the status register until the full flag is not set
    while (*MBOX_STATUS & MBOX_FULL) {
        asm volatile("nop");
    }
    // Write the data combined with the channel to the write register
    *MBOX_WRITE = r;

    /* Read to a mailbox*/
    while (1) {
        // Read the status register until the empty flag is not set
        while (*MBOX_STATUS & MBOX_EMPTY) {
            asm volatile("nop");
        }
        /* Is it a response to our message? */
        if (r == *MBOX_READ)
            /* Is it a valid successful response? */
            return mbox[1] == MBOX_RESPONSE;
    }
    return 0;
}
