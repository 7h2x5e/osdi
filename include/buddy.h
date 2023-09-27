#ifndef _BUDDY_H
#define _BUDDY_H

#include <include/types.h>

#define MAX_ORDER 11

struct free_area {
    struct list_head free_list;
    uint64_t nr_free;
};

struct buddy_system {
    struct free_area free_area[MAX_ORDER];
};

#endif