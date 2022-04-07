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
#include "gpio.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

void uart_init(unsigned int baudrate) {
/* 1. Initialize UART */
#define SYS_CLOCK_FREQ 2.5e8
  *AUX_ENABLE |= 1;   // Enable UART1, AUX mini uart
  *AUX_MU_CNTL = 0;   // Disable TX, TX and auto-flow control when configuring
  *AUX_MU_LCR = 3;    // Make UART works in 8-bit mode
  *AUX_MU_MCR = 0;    // Set RTS to high. Don't need auto-flow control
  *AUX_MU_IER = 0;    // Disable interrupt
  *AUX_MU_IIR = 0xc6; // No FIFO.
  *AUX_MU_BAUD =
      (unsigned int)((SYS_CLOCK_FREQ / 8 / baudrate) - 1); // Set baud rate

  /* 2. Map UART1 to GPIO pins */

  /*
   * Goal: Set GPIO14, 15 to ALT5
   * GPIO 0-9   => GPFSEL0
   * GPIO 10-19 => GPFSEL1
   * GPIO 20-29 => GPFSEL2
   * ...
   * GPIO14-15 use FSEL4 and FSEL5 field in GPFSEL1.
   */
  register unsigned int r;
  r = *GPFSEL1;
  r &= ~((7 << 12) | (7 << 15));
  r |= (2 << 12) | (2 << 15); // ALT5 = 010
  *GPFSEL1 = r;

  /* 2. Disable GPIO pull-up/down */
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
  *GPPUDCLK0 = 0; // Flush GPIO setup

  // 3. Enable Tx, Rx
  *AUX_MU_CNTL = 3;
}

void uart_putc(unsigned int c) {
  /* Wait until we can send */
  do {
    asm volatile("nop");
  } while (!(*AUX_MU_LSR & 0x20));
  *AUX_MU_IO = c;
}

char uart_getc() {
  /* wait until something is in the buffer */
  do {
    asm volatile("nop");
  } while (!(*AUX_MU_LSR & 0x01));
  char c = (char)(*AUX_MU_IO);
  return c == '\r' ? '\n' : c;
}

void uart_flush() {
  // Flush receive FIFO
  while (*AUX_MU_LSR & 0x01) {
    (*AUX_MU_IO);
  }
}

void uart_printf(char *fmt, ...) {
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
