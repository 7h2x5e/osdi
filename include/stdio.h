#ifndef STDIO_H
#define STDIO_H

#include <include/types.h>

// lib/printf.c
void printf(const char *fmt, ...);
unsigned int sprintf(char *dst, const char *fmt, ...);
unsigned int vsprintf(char *dst, const char *fmt, __builtin_va_list args);

// lib/puts.c
int putc(unsigned char c);
ssize_t fputs(const char *buf);

// lib/gets.c
int getc();
char *fgets(char *buf, size_t len);

#endif
