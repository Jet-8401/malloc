#include "libft_malloc.h"
#include <errno.h>

struct zone_info_s get_zone_infos(const size_t payload_size) {
    struct zone_info_s info;

    if (payload_size <= TINY_ZONE_TRESHOLD) {
        info.zone = &mctx.allocator.tiny_zone;
        info.type = TINY;
        info.lock = &mctx.tiny_lock;
    } else if (payload_size <= SMALL_ZONE_TRESHOLD) {
        info.zone = &mctx.allocator.small_zone;
        info.type = SMALL;
        info.lock = &mctx.small_lock;
    } else {
        info.zone = &mctx.allocator.large_zone;
        info.type = LARGE;
        info.lock = &mctx.large_lock;
    }

    return info;
}

void zone_push_back(zone_metadata_t **head, zone_metadata_t *zone) {
    zone->next = NULL;
    zone->prev = NULL;

    if (*head == NULL) {
        *head = zone;
    } else {
        zone_metadata_t *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = zone;
        zone->prev = current;
    }
}

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

#include <stdbool.h>

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
    bool was_found = false;
    zone_metadata_t *callback = NULL;

    if (_is_pointer_valid(mctx.allocator.tiny_zone, ptr, &callback))
        was_found = true;
    if (!was_found && _is_pointer_valid(mctx.allocator.small_zone, ptr, &callback))
        was_found = true;
    if (!was_found && _is_pointer_valid(mctx.allocator.large_zone, ptr, &callback))
        was_found = true;

    if (was_found) {
        *zone = callback;
        *chunk = (chunk_header_t*) ((uint8_t*) ptr - mctx.HEADER_SIZE);
    }

    return was_found;
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
