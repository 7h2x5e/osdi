#include "shell.h"
#include "c_string.h"
#include "irq.h"
#include "peripherals/mbox.h"
#include "peripherals/timer.h"
#include "peripherals/uart.h"
#include "time.h"
#include "utils.h"

#define BUFFER_MAX_SIZE 80
#define MAX_BOOTLOADER_SIZE 32768
#define ALIGN_uint(x, a) __ALIGN_MASK(x, ((unsigned int) (a)) - 1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

void loadimg()
{
    char str[BUFFER_MAX_SIZE];

    // Attempt to read load address
    int load_address = 0x80000;
    uart_printf("Start loading kernel image...\n");
    uart_printf("Please input 32-bit kernel load address (default: 80000): ");
    if (uart_fgets(str, BUFFER_MAX_SIZE) && str[0] != '\n') {
        str[strlen(str) - 1] = 0;  // Replace newline character with null byte
        if (htoi(str, &load_address) == -1)
            goto _error;
        load_address =
            ALIGN_uint(load_address,
                       16);  // align load address to a 16-byte boundary
    }

    // Input kernel size
    register int kernel_size = 0;
    int input_size;
    uart_printf("Please input kernel size: ");
    if (uart_fgets(str, BUFFER_MAX_SIZE) && str[0] != '\n') {
        str[strlen(str) - 1] = 0;
        if (atoi(str, &input_size) == -1)
            goto _error;
        kernel_size = input_size;
        uart_printf("Kernel image size: %d bytes, load address: 0x%h\n",
                    kernel_size, load_address);
    } else {
        goto _error;
    }

    // Relocate bootloader if needed
    extern char __loader_size;
    unsigned long loader_size = (unsigned long) &__loader_size;
    register char *kernel_start = (char *) (unsigned long) load_address,
                  *__new_start = kernel_start - MAX_BOOTLOADER_SIZE;
    if ((unsigned long) kernel_start < (0x80000 + loader_size) &&
        (unsigned long) (kernel_start + kernel_size) > 0x80000) {
        // Relocate bootloader. Assume bootloader size is less than
        // MAX_BOOTLOADER_SIZE bytes
        uart_printf("Relocating bootloader from 0x%h to 0x%h\n", 0x80000,
                    (int) (unsigned long) __new_start);
        char *src = (char *) 0x80000, *dst = __new_start;
        while (loader_size--)
            *dst++ = *src++;
        asm volatile("mov sp, %0" : : "r"(__new_start));
        goto *(&&_receive_kernel +
               ((unsigned long) kernel_start - MAX_BOOTLOADER_SIZE -
                0x80000));  // Calculate new address
    }

_receive_kernel:
    uart_printf("Please send kernel from UART now...\n");

    // Receive kernel data and validate it
    register int remain = kernel_size;
    char c;
    while (remain--) {
        c = (char) uart_getc_raw();
        *kernel_start++ = c;
        if (!(remain % 100))
            uart_printf("\r%d%%", 100 * (kernel_size - remain) / kernel_size);
    }

    uart_printf("\nSuccess! Load kernel...\n");
    asm volatile("blr %0" : : "r"(__new_start + MAX_BOOTLOADER_SIZE));

_error:
    // Shoult not reach here
    uart_printf("Abort!\n");
}

int search_command(char *str)
{
    static int irq = 0;
    if (!strcmp(str, "exit")) {
        uart_printf("entering infinite loop...");
        return -1;
    } else if (!strcmp(str, "hello")) {
        uart_printf("Hello world\n");
    } else if (!strcmp(str, "help")) {
        uart_printf("help: print all available commands\n");
        uart_printf("hello: print Hello world!\n");
        uart_printf("timestamp: print current timestamp\n");
        uart_printf("reboot: reboot rpi3 (doesn't work on QEMU)\n");
        uart_printf("loadimg: load new kernel\n");
        uart_printf("irq: enable/disable timer\n");
        uart_printf("exit: exit shell\n");
    } else if (!strcmp(str, "timestamp")) {
        uart_printf("[%f]\n", get_timestamp());
    } else if (!strcmp(str, "reboot")) {
        uart_printf("Rebooting...\n");
        reset(50);
        while (1)
            ;  // should not return
    } else if (!strcmp(str, "loadimg")) {
        loadimg();
    } else if (!strcmp(str, "irq")) {
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
    uart_printf("\n---Raspberry PI 3 B+---\n");
    get_board_revision();
    get_vc_mem();
    uart_printf("~$ ");

    char str[BUFFER_MAX_SIZE];
    while (uart_fgets(str, BUFFER_MAX_SIZE)) {
        str[strlen(str) - 1] = 0;  // Replace newline with null byte
        if (search_command(str) == -1)
            goto _exit;
        uart_printf("~$ ");
    }
_exit:
    return;
}