#include "uart.h"

void main(){
    uart_init(115200);
    uart_flush();
    char c;
    while((c = uart_getc())){
        uart_putc(c);
    }
    return;
}