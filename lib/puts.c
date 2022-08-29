#include <include/stdio.h>
#include <include/syscall.h>
#include <include/types.h>

int putc(unsigned char c)
{
    // Blocking write
    // Write a character to ring buffer and return the character
    // written as an unsigned char cast to an int.
    if (c == '\n') {
        while (1 != uart_write(&(char[1]){'\r'}, 1)) {
        }
    }
    while (1 != uart_write(&(char[1]){c}, 1)) {
    }
    return (int) c;
}


ssize_t fputs(const char *buf)
{
    // Attempts to write the string 'buf' without its terminating
    // null byte. Return a nonnegative number on success, or EOF
    // on error.
    size_t count = 0;
    while (buf[count]) {
        putc(buf[count++]);
    }
    return count;
}
