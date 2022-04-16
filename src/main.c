#include "shell.h"
#include "uart.h"
#include "fb.h"

void main() {
  uart_init(115200);
  uart_flush();
  fb_init();
  fb_showpicture();
  shell();
  return;
}