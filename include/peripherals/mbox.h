#ifndef MBOX_H
#define MBOX_H

#include <include/peripherals/gpio.h>

#define MBOX_BASE (MMIO_BASE + 0x0000B880)
#define MBOX_READ ((volatile unsigned int *) (MBOX_BASE + 0x0))
#define MBOX_POLL ((volatile unsigned int *) (MBOX_BASE + 0x10))
#define MBOX_SENDER ((volatile unsigned int *) (MBOX_BASE + 0x14))
#define MBOX_STATUS ((volatile unsigned int *) (MBOX_BASE + 0x18))
#define MBOX_CONFIG ((volatile unsigned int *) (MBOX_BASE + 0x1C))
#define MBOX_WRITE ((volatile unsigned int *) (MBOX_BASE + 0x20))
#define MBOX_RESPONSE 0x80000000
#define MBOX_FULL 0x80000000
#define MBOX_EMPTY 0x40000000

#define GET_BOARD_REVISION 0x00010002
#define GET_VC_MEMORY 0x00010006
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001

#define MBOX_REQUEST 0

/* channels */
#define MBOX_CH_POWER 0
#define MBOX_CH_FB 1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS 4
#define MBOX_CH_BTNS 5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7
#define MBOX_CH_PROP 8

/* tags */
#define MBOX_TAG_GETSERIAL 0x10004
#define MBOX_TAG_SETCLKRATE 0x00038002
#define MBOX_TAG_REQ 0x00000000
#define MBOX_TAG_LAST 0

/* framebuffer tags */
#define MBOX_TAG_SET_PHY_WH 0x48003
#define MBOX_TAG_SET_VIR_WH 0x48004
#define MBOX_TAG_SET_VIR_OFFSET 0x48009
#define MBOX_TAG_SET_DEPTH 0x48005
#define MBOX_TAG_SET_PIXEL_ORDER 0x48006
#define MBOX_TAG_ALLOCATE_BUFFER 0x40001
#define MBOX_TAG_GET_PITCH 0x40008

extern volatile unsigned int mbox[36];
int mbox_call(unsigned char ch);
void get_board_revision();
void get_vc_mem();

#endif