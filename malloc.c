#include "libft_malloc.h"
#include <stddef.h>

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

static void *_handle_large_alloc(size_t size) {

}

static void *_search_through_free_list(
    zone_metadata_t *zone,
    size_t min_size
) {
    freed_header_t *it;
    for (it = zone->begin; it != NULL; it = it->next) {
        size_t raw_it_size = UNMASK(it->size);
        if (raw_it_size < min_size)
            continue;

        chunk_header_t *found_chunk = (chunk_header_t*) it;
        found_chunk->size = raw_it_size; // don't inherit IS_PREV_FREE

        // check if there is remaining space to split the freed chunk
        size_t remaining_size = raw_it_size - min_size;
        if (remaining_size < MIN_FREED_CHUNK_SIZE) {
            // update the size of the found chunk
            found_chunk->size = min_size;

            freed_header_t *new_chunk = (void*) it + min_size;
            new_chunk->size = remaining_size;
        }
    }
}

static void *_search_free_chunk(zone_metadata_t *zone, size_t size) {
    // The `max` macro make sure that whatever the payload/size requested
    // by the user is, it will always at least be MIN_FREED_CHUNK_SIZE
    // to fit the freed chunk metadata once freed, therefore its range is:
    // [MIN_FREED_CHUNK_SIZE, ALIGNED_PAYLOAD + METADATA]
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
