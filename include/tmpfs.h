#ifndef _TMPFS_H
#define _TMPFS_H

#include <include/types.h>

#define TMPFS_FIEL_BUFFER_MAX_LEN 64

typedef struct tmpfs_node {
    size_t file_length;
    uint8_t buffer[TMPFS_FIEL_BUFFER_MAX_LEN];

} tmpfs_node_t;

void tmpfs_init();

#endif