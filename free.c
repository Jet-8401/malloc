#include "libft_malloc.h"

// Note: When freeing a point we need to check for the chunks surrounding it
// inside memory to coalesce it if needed.
void free(void *ptr) {
    freed_chunk_header_t *meta = ((void*) ptr) - CHUNK_HEADER_SIZE;
    const size_t size = meta->size & ~CHUNK_META_MASK;
    struct zone_info_s infos = _get_zone_infos(
        size
    );

    meta->next = (*infos.zone)->begin;
    (*infos.zone)->begin = meta;

    // if (flags & IS_PREV_FREE) {
    //     freed_chunk_header_t *previous = ((void*) meta) - CHUNK_FOOTER_SIZE;
    //     (void) previous;
    // }

    return;
}
