#ifndef _MMAN_H
#define _MMAN_H

#include <include/types.h>

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

#define MAP_FIXED 0x0010
#define MAP_ANONYMOUS 0x0020
#define MAP_POPULATE 0x8000

#define MAP_FAILED ((void *) -1)

void *do_mmap(void *, size_t, int, int, void *, int);

#endif