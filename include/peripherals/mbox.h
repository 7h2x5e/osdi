#ifndef MBOX_H
#define MBOX_H

#include "gpio.h"

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