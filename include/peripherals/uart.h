#ifndef UART_H
#define UART_H

#include <include/types.h>

#define EOF -1
#define UART_IRQ (1 << 25)

typedef struct ringbuf_t ringbuf_t;

/*
 * UART_INTERRUPT_MODE: Get/put character from/to uart buffer
 *   UART_POLLING_MODE: Get/put character from/to uart peripheral directly
 */
enum { UART_POLLING_MODE = false, UART_INTERRUPT_MODE = true };

void uart_init(unsigned int, bool);
void uart_flush();
void uart_handler();

/* uart_read & uart_write are used in system call */
ssize_t _uart_read(void *, size_t);
ssize_t _uart_write(void *, size_t);

#endif