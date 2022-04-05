#include "shell.h"
#include "c_string.h"
#include "uart.h"

#define LINEFEED 0x0A
#define BUFFER_MAX_SIZE 80

void help() {
  uart_printf("help: print all available commands\n");
  uart_printf("hello: print Hello world!\n");
}

void hello() { uart_printf("Hello world\n"); }

void search_command(char *buffer) {
  if (!*buffer)
    return;
  if (!strcmp(buffer, "hello")) {
    hello();
  } else if (!strcmp(buffer, "help")) {
    help();
  } else {
    uart_printf("Unknown command\n");
  }
}

void shell() {
  char c, buffer[BUFFER_MAX_SIZE];
  unsigned int count = 0;
  uart_printf("---Raspberry PI 3 B+---\n");
  uart_printf("~$ ");
  while ((c = uart_getc())) {
    switch (c) {
    case LINEFEED:
      uart_printf("\n");
      buffer[count] = 0;
      search_command(buffer);
      uart_printf("~$ ");
      count = 0;
      break;
    default:
      if (count + 1 < BUFFER_MAX_SIZE) {
        buffer[count++] = c;
        uart_putc(c);
      }
    }
  }
_exit:
  return;
}