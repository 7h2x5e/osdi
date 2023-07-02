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

#define MAP_FAILED ((uint64_t) -1)

uint64_t do_mmap(uint64_t addr,
                 uint64_t len,
                 uint32_t prot,
                 uint32_t flags,
                 uint64_t file_start,
                 uint64_t file_offset);
int32_t do_munmap(uint64_t addr, uint64_t length);

#endif