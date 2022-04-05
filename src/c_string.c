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