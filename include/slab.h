#ifndef _SLAB_H
#define _SLAB_H

#include <include/types.h>
#include <include/list.h>

struct slab {
    struct list_head free_list;
    struct list_head page_list;
    size_t object_size;
};

struct slab_object {
    struct list_head free_list;
    char ptr[0];
};

struct slab_page {
    struct {
        struct list_head page_list;  // 8 bytes
        size_t nr_free;              // 8 bytes
    } __attribute__((aligned(4)));   // 16 bytes
    char begin[0];
};

#define __SLAB_INITIALIZER(slabname, size)                                    \
    {                                                                         \
        .page_list = LIST_HEAD_INIT(slabname.page_list), .object_size = size, \
        .free_list = LIST_HEAD_INIT(slabname.free_list)                       \
    }

#define DEFINE_SLAB(slabname, size) \
    struct slab slabname = __SLAB_INITIALIZER(slabname, size)

struct kmalloc_info_struct {
    const char *name;
    unsigned int size;
    struct slab *slab;
};

void kfree(const void *);
void *kmalloc(size_t);
void *kzalloc(size_t);

#endif