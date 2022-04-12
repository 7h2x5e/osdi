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

extern volatile unsigned int mbox[36];
int mbox_call(unsigned char ch);
void get_board_revision();
void get_vc_mem();

#endif