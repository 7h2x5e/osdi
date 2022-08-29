#ifndef UTILS_H
#define UTILS_H

#include <include/types.h>

struct TimeStamp {
    uint64_t freq, counts;
};

int64_t do_reset(uint64_t tick);
int64_t do_cancel_reset();
int64_t do_get_timestamp(struct TimeStamp *);

#endif