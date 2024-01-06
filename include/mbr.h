#ifndef _MBR_H
#define _MBR_H

#include <include/types.h>

/* modified caldwell/gdisk/mbr.h */

struct chs {
    uint8_t cylinder;
    uint8_t head;
    uint8_t sector;
}; /* 3 bytes */

struct mbr_partition {
    uint8_t status;
    struct chs first_sector;
    uint8_t partition_type;
    struct chs last_sector;
    uint32_t first_sector_lba;
    uint32_t sectors;
} __attribute__((packed, aligned(__alignof__(uint16_t)))); /* 16 bytes */

struct mbr {
    uint8_t code[440];
    uint32_t disk_signature;
    uint16_t unused;
    struct mbr_partition partition[4]; /* 16*4=64 bytes*/
    uint16_t mbr_signature;
}; /* 512 bytes */

int parse_mbr(struct mbr *buffer, uint32_t *lba);

#endif