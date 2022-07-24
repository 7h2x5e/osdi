/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "c_string.h"
#include "peripherals/gpio.h"
#include "peripherals/mbox.h"

/* PL011 UART registers */
#define UART0_DR ((volatile unsigned int *) (MMIO_BASE + 0x00201000))
#define UART0_FR ((volatile unsigned int *) (MMIO_BASE + 0x00201018))
#define UART0_IBRD ((volatile unsigned int *) (MMIO_BASE + 0x00201024))
#define UART0_FBRD ((volatile unsigned int *) (MMIO_BASE + 0x00201028))
#define UART0_LCRH ((volatile unsigned int *) (MMIO_BASE + 0x0020102C))
#define UART0_CR ((volatile unsigned int *) (MMIO_BASE + 0x00201030))
#define UART0_IMSC ((volatile unsigned int *) (MMIO_BASE + 0x00201038))
#define UART0_ICR ((volatile unsigned int *) (MMIO_BASE + 0x00201044))

void uart_init(unsigned int baudrate)
{
    /* 1. Turn off UART0 */
    *UART0_CR = 0;

    /* 2. Set up clock for consistent divisor values */
    mbox[0] = 9 * 4;
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_SETCLKRATE;  // set clock rate
    mbox[3] = 12;                   // Buffer's length
    mbox[4] = MBOX_TAG_REQ;
    mbox[5] = 2;        // UART clock
    mbox[6] = 4000000;  // 4Mhz
    mbox[7] = 0;        // clear turbo
    mbox[8] = MBOX_TAG_LAST;
    mbox_call(MBOX_CH_PROP);

    /* 3. Map UART0 to GPIO pins
     *
     * Goal: Set GPIO14, 15 to ALT0
     * GPIO 0-9   => GPFSEL0
     * GPIO 10-19 => GPFSEL1
     * GPIO 20-29 => GPFSEL2
     * ...
     * GPIO14-15 use FSEL4 and FSEL5 field in GPFSEL1.
     */
    register unsigned int r;
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));
    r |= (4 << 12) | (4 << 15);  // ALT0 = 100
    *GPFSEL1 = r;

    /* 4. Disable GPIO pull-up/down */
    *GPPUD = 0;
    // Wait 150 cycles (required set-up time for the control signal)
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    // Wait 150 cycles (required hold time for the control signal)
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;  // Flush GPIO setup

    // 5. Enable Tx, Rx
    float buddiv = 4000000 / baudrate / 16;
    *UART0_ICR = 0x7FF;                   // clear interrupts
    *UART0_IBRD = (unsigned int) buddiv;  // 115200 baud, IBRD=0x2, FBRD=0xB
    *UART0_FBRD = (unsigned int) ((buddiv - *UART0_IBRD) * 64);
    *UART0_LCRH = 0x7 << 4;  // 8bits, enable FIFOs
    *UART0_CR = 0x301;       // enable Tx, Rx, UART
}

void uart_putc(unsigned int c)
{
    /* Wait until we can send */
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x20);
    *UART0_DR = c;
}

char uart_getc()
{
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x10);
    char c = (char) (*UART0_DR);
    return c == '\r' ? '\n' : c;
}

char uart_getc2()
{
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x10);
    return (char) (*UART0_DR);
}

void uart_flush()
{
    // Flush receive FIFO
    while (!(*UART0_FR & 0x10)) {
        (*UART0_DR);
    }
}

void uart_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    extern char _end;
    char *dst = &_end;
    vsprintf(dst, fmt, args);
    __builtin_va_end(args);

    while (*dst) {
        if (*dst == '\n')
            uart_putc('\r');
        uart_putc(*dst++);
    }
}
