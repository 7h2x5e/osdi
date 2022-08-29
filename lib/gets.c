#include <include/stdio.h>
#include <include/syscall.h>

int getc()
{
    // Blocking read
    // Reads a character from ring buffer and returns it as an
    // unsigned char cast to an int.
    unsigned char buf[1];
    while (1 != uart_read(buf, 1)) {
    }
    buf[0] = (buf[0] == '\r') ? '\n' : buf[0];
    // Echo back with the character.
    putc(buf[0]);
    return (int) buf[0];
}

char *fgets(char *buf, size_t len)
{
    // Attempts to read up to one less than 'len' characters.
    // Reading stops after a newline. If a newline is read, it
    // is stored into the buffer. A terminating null byte '\0'
    // is stored after the last character in the buffer.
    // Return 'buf' on success or NULL on error.
    size_t idx = 0;
    char c;
    if (len < 2)
        return NULL;
    do {
        c = getc();
        buf[idx++] = c;
    } while (c != '\n' && idx < (len - 1));
    buf[idx] = '\0';
    return buf;
}