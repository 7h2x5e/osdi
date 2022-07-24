#include "fb.h"
#include "peripherals/mbox.h"
#include "peripherals/uart.h"

static unsigned int width, height, pitch, isrgb;
static void *fb;

void fb_init()
{
    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST;

    /* Set physical width/height */
    mbox[2] = MBOX_TAG_SET_PHY_WH;
    mbox[3] = 8;
    mbox[4] = MBOX_TAG_REQ;
    mbox[5] = 1024;  // width
    mbox[6] = 768;   // height

    /* Set virtual width/height */
    mbox[7] = MBOX_TAG_SET_VIR_WH;
    mbox[8] = 8;
    mbox[9] = MBOX_TAG_REQ;
    mbox[10] = 1024;  // virtual width
    mbox[11] = 768;   // virtual height

    /* Set virtual offset */
    mbox[12] = MBOX_TAG_SET_VIR_OFFSET;
    mbox[13] = 8;
    mbox[14] = MBOX_TAG_REQ;
    mbox[15] = 0;  // X offset
    mbox[16] = 0;  // Y offset

    /* Set depth */
    mbox[17] = MBOX_TAG_SET_DEPTH;
    mbox[18] = 4;
    mbox[19] = MBOX_TAG_REQ;
    mbox[20] = 32;  // bits per pixel

    /* Set pixel order */
    mbox[21] = MBOX_TAG_SET_PIXEL_ORDER;
    mbox[22] = 4;
    mbox[23] = MBOX_TAG_REQ;
    mbox[24] = 1;  // 0x0: BGR, 0x1: RGB

    /* Allocate framebuffer */
    mbox[25] = MBOX_TAG_ALLOCATE_BUFFER;
    mbox[26] = 8;
    mbox[27] = MBOX_TAG_REQ;
    mbox[28] = 4096;  // 1. alignment (req) 2. frame buffer base address (res)
    mbox[29] = 0;     // frame buffer size (res)

    /* Get pitch */
    mbox[30] = MBOX_TAG_GET_PITCH;
    mbox[31] = 4;
    mbox[32] = MBOX_TAG_REQ;
    mbox[33] = 0;  // res: bytes per line

    mbox[34] = MBOX_TAG_LAST;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mbox_call(MBOX_CH_PROP) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF;  // convert GPU address to ARM address
        width = mbox[5];         // get actual physical width
        height = mbox[6];        // get actual physical height
        pitch = mbox[33];        // get number of bytes per line
        isrgb = mbox[24];        // get the actual channel order
        fb = (void *) ((unsigned long) mbox[28]);
    } else {
        uart_printf("Unable to set screen resolution to 1024x768x32\n");
    }
}

void fb_showpicture()
{
    /* Safety check */
    if (!fb)
        return;

#define WHITE 0xFFFFFFFF
#define BLACK 0x0
#define BLOCK_SIZE 1 << 6  // 64x64 pixel
    unsigned int x_count = 0, y_count = 0, acc = 0;
    for (void *y = fb; y < fb + height * pitch; y += pitch, ++y_count) {
        if (y_count & BLOCK_SIZE) {
            y_count = 0;
            acc++;
        }
        for (unsigned int *xy = (unsigned int *) y;
             xy < (unsigned int *) y + (pitch >> 2); xy += 1, ++x_count) {
            if (x_count & BLOCK_SIZE) {
                x_count = 0;
                acc++;
            }
            *xy = acc & 1 ? WHITE : BLACK;
        }
    }

#undef BLOCK_SIZE
#undef WHITE
#undef BLACK
}