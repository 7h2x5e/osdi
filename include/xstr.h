#ifndef _XSTR_H
#define _XSTR_H

#include <include/types.h>

typedef struct _xstr {
    char *content;
    size_t length, capacity;
} xstr_t;

xstr_t *xstr_create(size_t);
void xstr_destroy(xstr_t *);
void xstr_resize(xstr_t *, size_t);
void xstr_append(xstr_t *, char *);
void xstr_clear(xstr_t *);
char *xstr_get(xstr_t *);
xstr_t *xstr_reverse(xstr_t *);

#endif