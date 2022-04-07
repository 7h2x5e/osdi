#include "shell.h"
#include "c_string.h"
#include "time.h"
#include "uart.h"
#include "utils.h"

#define LINEFEED 0x0A
#define BUFFER_MAX_SIZE 80

void help() {
  uart_printf("help: print all available commands\n");
  uart_printf("hello: print Hello world!\n");
  uart_printf("timestamp: print current timestamp\n");
  uart_printf("reboot: reboot rpi3 (doesn't work on QEMU)\n");
  uart_printf("exit: exit shell\n");
}

void hello() { uart_printf("Hello world\n"); }

void timestamp() { uart_printf("[%f]\n", getTime()); }

void reboot() {
  uart_printf("Rebooting...\n");
  reset(50);
  while (1)
    ; // should not return
}

void exit() { uart_printf("entering infinite loop..."); }

int search_command(char *buffer) {
  int ret = 0;
  if (!strcmp(buffer, "hello")) {
    hello();
  } else if (!strcmp(buffer, "help")) {
    help();
  } else if (!strcmp(buffer, "timestamp")) {
    timestamp();
  } else if (!strcmp(buffer, "reboot")) {
    reboot();
  } else if (!strcmp(buffer, "exit")) {
    exit();
    ret = -1;
  } else {
    uart_printf("Unknown command\n");
  }
  return ret;
}

void shell() {
  char c, buffer[BUFFER_MAX_SIZE];
  unsigned int count = 0;
  uart_printf("\n---Raspberry PI 3 B+---\n~$ ");
  while ((c = uart_getc())) {
    switch (c) {
    case LINEFEED:
      uart_printf("\n");
      buffer[count] = 0;
      if (search_command(buffer) == -1)
        goto _exit;
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