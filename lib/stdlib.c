#include <include/stdlib.h>

int atoi(const char *str, int *dst)
{
    long int sum = 0;
    char c;
    int sign = 0;
    if (*str == '-') {
        str++;
        sign = 1;
    }
    while ((c = *str++)) {
        c -= '0';
        if (c > 9 || c < 0)
            return -1;  // return -1 on failure
        sum *= 10;
        sum += c;
    }
    *dst = sign == 1 ? (int) (-sum) : (int) sum;
    return 0;
}

int htoi(const char *str, int *dst)
{
    long int sum = 0;
    char c, d;
    while ((c = *str++)) {
        if (c >= 'a' && c <= 'e') {
            d = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'E') {
            d = c - 'A' + 10;
        } else if (c >= '0' && c <= '9') {
            d = c - '0';
        } else
            return -1;  // return -1 on failure
        sum = (sum << 4) | d;
    }
    *dst = (int) sum;
    return 0;
}

/*  bztsrc: raspi3-tutorial/12_printf/sprintf.c */
char *itoa(char *tmpstr, long int arg)
{
    int sign = 0, i = 18;
    // check input
    if ((int) arg < 0) {
        arg *= -1;
        sign++;
    }
    if (arg > 99999999999999999L) {
        arg = 99999999999999999L;
    }
    // convert to string
    tmpstr[i] = 0;
    do {
        tmpstr[--i] = '0' + (arg % 10);
        arg /= 10;
    } while (arg != 0 && i > 0);
    if (sign) {
        tmpstr[--i] = '-';
    }
    return &tmpstr[i];
}

char *itoh(char *tmpstr, long int arg)
{
    int i = 18;
    // convert to string
    tmpstr[i] = 0;
    do {
        int tmp = arg & 0xF;
        if (tmp / 10)
            tmpstr[--i] = 'A' + tmp % 10;
        else
            tmpstr[--i] = '0' + tmp;
        arg >>= 4;
    } while (arg != 0 && i > 0);
    return &tmpstr[i];
}