#ifndef _ASSERT_H
#define _ASSERT_H

void _warn(const char *, int, const char *fmt, const char *str);
void _panic(const char *, int, const char *fmt, const char *str)
    __attribute__((noreturn));

#define warn(...) _warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)                              \
    do {                                       \
        if (!(x))                              \
            panic("assertion failed: %s", #x); \
    } while (0)

#endif
