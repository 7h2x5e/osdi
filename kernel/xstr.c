#include <include/xstr.h>
#include <include/slab.h>
#include <include/string.h>

xstr_t *xstr_create(size_t capacity)
{
    if (!capacity) {
        capacity = 1;
    }
    capacity = 1UL << (64 - __builtin_clzl(capacity));

    xstr_t *xstr = kmalloc(sizeof(xstr_t));
    xstr->length = 0;
    xstr->capacity = capacity;
    xstr->content = (char *) kzalloc(capacity);
    return xstr;
}

void xstr_destroy(xstr_t *xstr)
{
    kfree(xstr->content);
    kfree(xstr);
}

void xstr_resize(xstr_t *xstr, size_t new_capacity)
{
    if (new_capacity < xstr->capacity)
        return;
    new_capacity = 1UL << (64 - __builtin_clzl(new_capacity));

    char *new_content = (char *) kmalloc(new_capacity);
    strcpy(new_content, xstr->content);
    kfree(xstr->content);
    xstr->content = new_content;
    xstr->capacity = new_capacity;
}

void xstr_append(xstr_t *xstr, char *buf)
{
    size_t len = strlen(buf);
    if (xstr->length + len + 1 > xstr->capacity) {
        xstr_resize(xstr, xstr->length + len + 1);
    }
    strcpy(xstr->content + xstr->length, buf);
    xstr->length += len;
    xstr->content[xstr->length] = 0;
}

void xstr_clear(xstr_t *xstr)
{
    xstr->length = 0;
}

char *xstr_get(xstr_t *xstr)
{
    char *buf = kzalloc(xstr->length + 1);
    strncpy(buf, xstr->content, xstr->length);
    xstr_clear(xstr);
    return buf;
}

xstr_t *xstr_reverse(xstr_t *xstr)
{
    for (int i = 0; i < xstr->length / 2; i++) {
        char t = xstr->content[i];
        xstr->content[i] = xstr->content[xstr->length - 1 - i];
        xstr->content[xstr->length - 1 - i] = t;
    }
    return xstr;
}