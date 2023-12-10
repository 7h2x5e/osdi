#ifndef STRING_H
#define STRING_H

#include <include/types.h>

int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
int strlen(const char *);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *s1, const void *s2, size_t n);

#endif
