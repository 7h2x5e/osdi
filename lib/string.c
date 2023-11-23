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

char *strcpy(char *dest, const char *src)
{
    char c;
    char *s = (char *) src, *d = dest;

    do {
        c = *s++;
        *d++ = c;
    } while (c != '\0');

    return dest;
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

void *memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *) s;
    uint8_t *end = p + n;

    while (p != end) {
        *p++ = c;
    }

    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *byte1 = (const uint8_t *) s1;
    const uint8_t *byte2 = (const uint8_t *) s2;
    while ((*byte1 == *byte2) && (n > 0)) {
        ++byte1;
        ++byte2;
        --n;
    }

    if (n == 0) {
        return 0;
    }
    return *byte1 - *byte2;
}