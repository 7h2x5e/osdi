#include <include/peripherals/uart.h>
#include <include/printk.h>
#include <include/stdio.h>
#include <include/types.h>
#include <include/utils.h>

static inline int putc_k(unsigned char c)
{
    if (c == '\n') {
        while (1 != _uart_write(&(char[1]){'\r'}, 1)) {
        }
    }
    while (1 != _uart_write(&(char[1]){c}, 1)) {
    }
    return (int) c;
}


static inline ssize_t fputs_k(const char *buf)
{
    size_t count = 0;
    while (buf[count]) {
        putc_k(buf[count++]);
    }
    return count;
}


void printk(const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    extern char _end;
    char *dst = &_end;
    vsprintf(dst, fmt, args);
    __builtin_va_end(args);

    fputs_k(dst);
}

void printk_time(const char *fmt, ...)
{
    struct TimeStamp ts;
    do_get_timestamp(&ts);
    uint64_t freq = ts.freq, counts = ts.counts;
    printk("[%f] ", (float) counts / freq);

    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    extern char _end;
    char *dst = &_end;
    vsprintf(dst, fmt, args);
    __builtin_va_end(args);

    fputs_k(dst);
}