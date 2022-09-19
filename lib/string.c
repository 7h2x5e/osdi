#include <include/string.h>

/* Glibc: string/strcmp.c */
int strcmp(const char *p1, const char *p2)
{
    register const char *s1 = p1;
    register const char *s2 = p2;
    unsigned char c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    return c1 - c2;
}

int strlen(const char *str)
{
    int count = 0;
    while (*str++)
        ++count;
    return count;
}

/* copied from pdoane/osdev */
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *p = (uint8_t *) src;
    uint8_t *q = (uint8_t *) dst;
    uint8_t *end = p + n;

    while (p != end) {
        *q++ = *p++;
    }

    return dst;
}


void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *p = (uint8_t *) src;
    uint8_t *q = (uint8_t *) dst;
    uint8_t *end = p + n;

    if (q > p && q < end) {
        p = end;
        q += n;

        while (p != src) {
            *--q = *--p;
        }
    } else {
        while (p != end) {
            *q++ = *p++;
        }
    }

    return dst;
}