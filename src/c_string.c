#include "c_string.h"

/* Glibc: string/strcmp.c */
int strcmp(const char *p1, const char *p2) {
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

/*  bztsrc: raspi3-tutorial/12_printf/sprintf.c */
static inline char *itoa(char *tmpstr, long int arg) {
  int sign = 0, i = 18;
  // check input
  if ((int)arg < 0) {
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

static inline char *itoh(char *tmpstr, long int arg) {
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

unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args) {
  long int arg;
  char *p, *orig = dst, tmpstr[39]; // tmpstr = integer part (18 digits) +
                                    // decimal point (1digits) +
                                    // fractional part (18 digits)

  // failsafes
  if (dst == (void *)0 || fmt == (void *)0) {
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
        *dst++ = (char)arg;
        fmt++;
        continue;
      } else if (*fmt == 'd') {
        // decimal number
        arg = __builtin_va_arg(args, int);
        p = itoa(tmpstr, arg);
        goto copystring;
      } else if (*fmt == 'h') {
        // hex number
        arg = __builtin_va_arg(args, int);
        p = itoh(tmpstr, arg);
        goto copystring;
      } else if (*fmt == 'f') {
        // float
        float f = (float)__builtin_va_arg(args, double);
        // represent float by two integer parts: integer part (18 digits) +
        // fractional part (6 digits)
        long int integer, fraction;
        integer = (long int)f;
        f = f < 0 ? -f : f;
        fraction = (long int)(f * 1000000) % 1000000;
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
        if (p == (void *)0) {
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

unsigned int sprintf(char *dst, char *fmt, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, fmt);
  return vsprintf(dst, fmt, args);
}