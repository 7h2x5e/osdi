#ifndef UART_H
#define UART_H

#include "types.h"

#define EOF -1
#define UART_IRQ (1 << 25)
typedef struct ringbuf_t ringbuf_t;

void uart_init(unsigned int);
ssize_t uart_read(ringbuf_t *, void *, size_t);
ssize_t uart_write(ringbuf_t *, void *, size_t);

int uart_getc();
int uart_getc_raw();
int uart_putc(unsigned char);
char *uart_fgets(char *, size_t);
ssize_t uart_fputs(const char *);
void uart_printf(char *str, ...);
void uart_flush();
void uart_handler();

#endif