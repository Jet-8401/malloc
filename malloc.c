#include "libft_malloc.h"
#include <stddef.h>
#include <strings.h>

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

static void *_handle_large_alloc(size_t size) {

}

// `size` must be the aligned.
static void *_search_through_free_list(
    zone_metadata_t *zone,
    size_t size
) {
    freed_header_t *it;
    for (it = zone->begin; it != NULL; it = it->next) {
        size_t raw_it_size = UNMASK(it->size);
        if (raw_it_size < size)
            continue;

        // don't inherit the flags because two freed chunk would never be next
        // to each other in memory
        it->size = raw_it_size;
        _remove_from_free_list(&zone->begin, it);

        // check if there is remaining space to split the freed chunk
        size_t remaining_size = raw_it_size - size;
        if (remaining_size < MIN_FREED_CHUNK_SIZE) {
            // update the next chunk in memory to remove IS_PREV_FREE flag
            chunk_header_t *fw_chunk = (void*) it + raw_it_size;
            fw_chunk->size &= ~(size_t)IS_PREV_FREE;
        } else {
            // update the size of the found chunk
            it->size = size;

            // create new freed chunk inside the remaining space area
            freed_header_t *new_chunk = (void*) it + size;
            bzero(new_chunk, sizeof(freed_header_t)); // reset metadata
            new_chunk->size = remaining_size;

            // create footer for backward coalescing
            size_t footer_offset = new_chunk->size - sizeof(freed_footer_t);
            freed_footer_t *footer = (void*) new_chunk + footer_offset;
            footer->prev_size = new_chunk->size;

            _free_list_push_front(zone, new_chunk);
        }

        return (void*) it + CHUNK_HEADER_SIZE;
    }
    return NULL;
}

// `size` must be the aligned.
static void *_carve_space(zone_metadata_t *zone, size_t size) {

}


static void *_search_free_chunk(zone_metadata_t *zone, size_t size) {
    // The `max` macro make sure that whatever the payload/size requested
    // by the user is, it will always at least be MIN_FREED_CHUNK_SIZE
    // to fit the freed chunk metadata once freed, therefore its range is:
    // [MIN_FREED_CHUNK_SIZE, ALIGNED_PAYLOAD + ALIGNED_METADATA]
    const size_t min_size = max(
        ALIGN(size) + CHUNK_HEADER_SIZE, MIN_FREED_CHUNK_SIZE
    );

    void *chunk;
    zone_metadata_t *zone_it;
    for (zone_it = zone; zone_it != NULL; zone_it = zone_it->next) {
        if (!zone_it->begin)
            continue;
        chunk = _search_through_free_list(zone_it, min_size);
        if (chunk)
            return chunk;
        chunk = _carve_space(zone_it, min_size);
    }

    return NULL;
}

void *malloc(size_t size) {
    if (size == 0)
        return NULL;

    pthread_mutex_lock(&g_mutex);

    struct zone_info_s inf = _get_zone_infos(size);
    if (inf.type == LARGE)
        return _handle_large_alloc(size);

    void *chunk = _search_free_chunk(*inf.zone, size);
    if (chunk)
        return pthread_mutex_unlock(&g_mutex), chunk;

    return NULL;
}
