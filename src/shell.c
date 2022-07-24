#include "shell.h"
#include "c_string.h"
#include "irq.h"
#include "peripherals/mbox.h"
#include "peripherals/timer.h"
#include "peripherals/uart.h"
#include "time.h"
#include "utils.h"

#define LINEFEED 0x0A
#define BUFFER_MAX_SIZE 80
#define ALIGN_uint(x, a) __ALIGN_MASK(x, ((unsigned int) (a)) - 1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

void loadimg()
{
    unsigned char c;
    int load_address;
    register int kernel_size = 0;

    uart_printf("Start loading kernel image...\n");
    uart_printf("Please input 32-bit kernel load address (default: 80000): ");

    // Input load address
    char buf[BUFFER_MAX_SIZE];
    int idx = 0;
    while ((c = uart_getc())) {
        uart_printf("%c", c);
        if (c == LINEFEED)
            break;
        buf[idx++] = c;
    }
    buf[idx] = 0;
    if (!idx) {
        load_address = 0x80000;
    } else if (htoi(buf, &load_address) == -1)
        goto _error;
    load_address = ALIGN_uint(load_address,
                              16);  // align load address to a 16-byte boundary

    uart_printf("Please input kernel size: ");

    // Input kernel size in little-endian order
    for (int i = 0; i < 4; ++i)
        kernel_size |= (((int) uart_getc2()) << (i << 3));
    if (kernel_size <= 0)
        goto _error;

    uart_printf("%d\n", kernel_size);
    uart_printf("Kernel image size: %d bytes, load address: 0x%h\n",
                kernel_size, load_address);

    // Relocate bootloader if needed
    extern char __loader_size;
    unsigned long loader_size = (unsigned long) &__loader_size;
    register char *kernel_start = (char *) (unsigned long) load_address,
                  *__new_start = kernel_start - 8192;
    if ((unsigned long) kernel_start < (0x80000 + loader_size) &&
        (unsigned long) (kernel_start + kernel_size) > 0x80000) {
        // Relocate bootloader. Assume bootloader size is less than 8192 bytes
        uart_printf("Relocating bootloader from 0x%h to 0x%h\n", 0x80000,
                    (int) (unsigned long) __new_start);
        char *src = (char *) 0x80000, *dst = __new_start;
        while (loader_size--)
            *dst++ = *src++;
        asm volatile("mov sp, %0" : : "r"(__new_start));
        goto *(&&_receive_kernel + ((unsigned long) kernel_start - 8192 -
                                    0x80000));  // Calculate new address
    }

_receive_kernel:
    uart_printf("Please send kernel from UART now...\n");

    // Receive kernel data and validate it
    register int remain = kernel_size;
    while (remain--) {
        c = uart_getc2();
        *kernel_start++ = c;
        if (!(remain % 100))
            uart_printf("\r%d%%", 100 * (kernel_size - remain) / kernel_size);
    }

    uart_printf("\nSuccess! Load kernel...\n");
    asm volatile("blr %0" : : "r"(__new_start + 8192));

_error:
    // Shoult not reach here
    uart_printf("Abort!\n");
}

int search_command(char *buffer)
{
    static int irq = 0;
    if (!strcmp(buffer, "exit")) {
        uart_printf("entering infinite loop...");
        return -1;
    } else if (!strcmp(buffer, "hello")) {
        uart_printf("Hello world\n");
    } else if (!strcmp(buffer, "help")) {
        uart_printf("help: print all available commands\n");
        uart_printf("hello: print Hello world!\n");
        uart_printf("timestamp: print current timestamp\n");
        uart_printf("reboot: reboot rpi3 (doesn't work on QEMU)\n");
        uart_printf("loadimg: load new kernel\n");
        uart_printf("irq: enable/disable timer\n");
        uart_printf("exit: exit shell\n");
    } else if (!strcmp(buffer, "timestamp")) {
        uart_printf("[%f]\n", get_timestamp());
    } else if (!strcmp(buffer, "reboot")) {
        uart_printf("Rebooting...\n");
        reset(50);
        while (1)
            ;  // should not return
    } else if (!strcmp(buffer, "loadimg")) {
        loadimg();
    } else if (!strcmp(buffer, "irq")) {
        (irq ^= 0x1) ? uart_printf("Enable irq\n")
                     : uart_printf("Disable irq\n");
        toggle_irq(irq);
    } else {
        uart_printf("Unknown command\n");
    }
    return 0;
}

void shell()
{
    char c, buffer[BUFFER_MAX_SIZE];
    unsigned int count = 0;
    uart_printf("\n---Raspberry PI 3 B+---\n");
    get_board_revision();
    get_vc_mem();
    uart_printf("~$ ");
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