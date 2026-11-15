#ifndef CYNE_ARENA_H
#define CYNE_ARENA_H

#include <stdio.h>

#include "error.h"

typedef struct Arena {
    size_t size;
    size_t offset;
    unsigned char* buffer;
} Arena;

static inline CyneResult Arena_init(Arena* a, unsigned char* buffer, size_t size) {
    a->buffer = buffer;
    a->offset = 0;
    a->size = size;

    return SUCCESS;
}

static inline CyneResult Arena_reset(Arena* a) {
    a->offset = 0;
    return SUCCESS;
}

static inline CyneResult Arena_align_up(Arena* a, size_t alignment, size_t* out_aligned) {
    *out_aligned = (((size_t)a->buffer + a->offset + alignment - 1) & ~(alignment - 1)) - (size_t)a->buffer;
    return SUCCESS;
}

/* End of header */
#endif