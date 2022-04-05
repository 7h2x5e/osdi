#include "shell.h"
#include "uart.h"

void main() {
  uart_init(115200);
  uart_flush();
  shell();
  return;
}