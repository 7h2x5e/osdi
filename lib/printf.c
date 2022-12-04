#include <include/stdio.h>
#include <include/stdlib.h>

void printf(const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    extern char _end;
    char *dst = &_end;
    vsprintf(dst, fmt, args);
    __builtin_va_end(args);

    fputs(dst);
}

unsigned int vsprintf(char *dst, const char *fmt, __builtin_va_list args)
{
    long int arg;
    char *p, *orig = dst, tmpstr[39];  // tmpstr = integer part (18 digits) +
                                       // decimal point (1digits) +
                                       // fractional part (18 digits)

    // failsafes
    if (dst == (void *) 0 || fmt == (void *) 0) {
        return 0;
    }

    // main loop
    arg = 0;
    while (*fmt) {
        // argument access
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '%') {
                goto put;
            }
            if (*fmt == 'c') {
                // character
                arg = __builtin_va_arg(args, int);
                *dst++ = (char) arg;
                fmt++;
                continue;
            } else if (*fmt == 'd') {
                // decimal number
                arg = __builtin_va_arg(args, int);
                p = itoa(tmpstr, arg);
                goto copystring;
            } else if (*fmt == 'h') {
                // hex number
                arg = __builtin_va_arg(args, unsigned long long);
                p = itoh(tmpstr, arg);
                goto copystring;
            } else if (*fmt == 'f') {
                // float
                float f = (float) __builtin_va_arg(args, double);
                // represent float by two integer parts: integer part (18
                // digits) + fractional part (6 digits)
                long int integer, fraction;
                integer = (long int) f;
                f = f < 0 ? -f : f;
                fraction = (long int) (f * 1000000) % 1000000;
                p = itoa(&tmpstr[0], integer);
                char *p2 = itoa(&tmpstr[19], fraction);
                // concentrate two strings
                char *start = p;
                while (*start)
                    start++;
                if (*p2)
                    *(start++) = '.';
                while (*p2) {
                    *(start++) = *(p2++);
                }
                *start = '\0';
                goto copystring;
            } else if (*fmt == 's') {
                p = __builtin_va_arg(args, char *);
            copystring:
                if (p == (void *) 0) {
                    p = "(null)";
                }
                while (*p) {
                    *dst++ = *p++;
                }
            }
        } else {
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    *dst = 0;
    // number of bytes written
    return dst - orig;
}

unsigned int sprintf(char *dst, const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return vsprintf(dst, fmt, args);
}