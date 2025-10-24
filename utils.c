#include "libft_malloc.h"
#include <errno.h>

void remove_from_list(freed_header_t **head, freed_header_t *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else if (node == *head) {
        *head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }

    node->next = NULL;
    node->prev = NULL;
}

static bool _is_pointer_valid(
    zone_metadata_t *zone,
    void *ptr,
    zone_metadata_t **zone_found
) {
    for (; zone != NULL; zone = zone->next) {
        void *zone_payload = (uint8_t*) zone + mctx.ALIGNED_ZONE_METADATA;
        void *zone_end = (uint8_t*) zone + zone->size - mctx.HEADER_SIZE;
        if (ptr < zone_payload || ptr > zone_end)
            continue;

        if (zone_found)
            *zone_found = zone;
        return true;
    }
    return false;
}

bool search_pointer_in_heap(
    void *ptr,
    zone_metadata_t **zone,
    chunk_header_t **chunk
) {
    bool found = false;
    zone_metadata_t *callback = NULL;

    if (_is_pointer_valid(mctx.allocator.tiny_zone, ptr, &callback))
        found = true;
    if (!found && _is_pointer_valid(mctx.allocator.small_zone, ptr, &callback))
        found = true;
    if (!found && _is_pointer_valid(mctx.allocator.large_zone, ptr, &callback))
        found = true;

    if (found) {
        *zone = callback;
        *chunk = (chunk_header_t*) ((uint8_t*) ptr - mctx.HEADER_SIZE);
    }

    return found;
}

int compute_chunk_size(size_t user_size, size_t *chunk_size) {
    // check ALIGN won't overflow
    if (user_size > SIZE_MAX - (MEM_ALIGNMENT - 1)) {
        errno = ENOMEM;
        return -1;
    }

    // check adding header won't overflow
    size_t aligned = ALIGN(user_size);
    if (aligned > SIZE_MAX - mctx.HEADER_SIZE) {
        errno = ENOMEM;
        return -1;
    }

    // make chunk be at least MIN_CHUNK_SIZE to fit the freed chunk
    // metadata once freed
    size_t chunk = aligned + mctx.HEADER_SIZE;
    if (chunk < mctx.MIN_CHUNK_SIZE)
        chunk = mctx.MIN_CHUNK_SIZE;

    *chunk_size = chunk;
    return 0;
}
