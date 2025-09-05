#include "libft_malloc.h"

// Note: When freeing a point we need to check for the chunks surrounding it
// inside memory to coalesce it if needed.
void free(void *ptr) {
    chunk_header_t* meta = (void*) ptr - ALIGN(sizeof(chunk_header_t));
    char flags = meta->payload_size & CHUNK_META_MASK;

    if (flags & IS_PREV_FREE) {
        freed_chunk_header_t *previous = ((void*) meta) - CHUNK_FOOTER_SIZE;
        (void) previous;
    }

    return;
}
