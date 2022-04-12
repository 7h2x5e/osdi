#include "mbox.h"
#include "uart.h"

volatile unsigned int __attribute__((aligned(16))) mbox[36];

#define MBOX_BASE (MMIO_BASE + 0x0000B880)
#define MBOX_READ ((volatile unsigned int *)(MBOX_BASE + 0x0))
#define MBOX_POLL ((volatile unsigned int *)(MBOX_BASE + 0x10))
#define MBOX_SENDER ((volatile unsigned int *)(MBOX_BASE + 0x14))
#define MBOX_STATUS ((volatile unsigned int *)(MBOX_BASE + 0x18))
#define MBOX_CONFIG ((volatile unsigned int *)(MBOX_BASE + 0x1C))
#define MBOX_WRITE ((volatile unsigned int *)(MBOX_BASE + 0x20))
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

/*
 * Returns 0 on failure, non-zero on success.
 */
int mbox_call(unsigned char ch) {
  /* Write to a mailbox*/
  register unsigned int r =
      (unsigned int)(((unsigned long)&mbox) & (~0xF)) | (ch & 0xF);
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

#define GET_BOARD_REVISION 0x00010002
#define GET_VC_MEMORY 0x00010006
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001

void get_board_revision() {
  mbox[0] = 7 * 4;              // length of the message
  mbox[1] = MBOX_REQUEST;       // tags begin
  mbox[2] = GET_BOARD_REVISION; // tag identifier
  mbox[3] = 4; // maximum of request and response value buffer's length.
  mbox[4] = MBOX_TAG_REQ;
  mbox[5] = 0;             // value buffer
  mbox[6] = MBOX_TAG_LAST; // tags end

  if (mbox_call(MBOX_CH_PROP)) {
    uart_printf("Board revision: 0x%h\n", mbox[5]);
  } else {
    uart_printf("Unable to retrieve board revision.\n");
  };
}

void get_vc_mem() {
  mbox[0] = 8 * 4;
  mbox[1] = MBOX_REQUEST;
  mbox[2] = GET_VC_MEMORY;
  mbox[3] = 8;
  mbox[4] = MBOX_TAG_REQ;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = MBOX_TAG_LAST;

  if (mbox_call(MBOX_CH_PROP)) {
    uart_printf("VC core base address: 0x%h Size: 0x%h\n", mbox[5], mbox[6]);
  } else {
    uart_printf("Unable to retrieve address.\n");
  };
}