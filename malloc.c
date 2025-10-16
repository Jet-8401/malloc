#include "libft_malloc.h"
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>
#include <stdalign.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* Allocate new zone and return its address.
* Return NULL in case of error and set errno.
*/
static zone_metadata_t* _allocate_zone(
    size_t size,
    const enum ZONE_TYPE type
) {
    switch (type) {
        case TINY:
            size = TINY_ZONE_ALLOC_SIZE;
            break;
        case SMALL:
            size = SMALL_ZONE_ALLOC_SIZE;
            break;
        case LARGE:
            size += CHUNK_HEADER_SIZE;
            break;
        default:
            break;
    }

    size += ALIGNED_ZONE_METADATA;

    // Align the space required to the page alignment.
    size = ALIGN_TO(size, sysconf(_SC_PAGESIZE));

    zone_metadata_t *zone = mmap(
        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0
    );
    if (zone == MAP_FAILED) {
        exit(EXIT_FAILURE);
        return NULL;
    }

    zone->size = size;
    zone->next = NULL;
    zone->begin = NULL;

    zone->top = (void*) zone + ALIGNED_ZONE_METADATA;
    zone->top->size = (void*) zone + zone->size - (void*) zone->top;

    return zone;
}

static void _zone_push_back(zone_metadata_t **origin, zone_metadata_t *node) {
    if (*origin == NULL) {
        *origin = node;
    } else {
        zone_metadata_t *it = *origin;
        while (it->next != NULL) it = it->next;
        it->next = node;
    }
}

static void *_handle_large_alloc(zone_metadata_t **zone, size_t size) {
    zone_metadata_t *new_zone = _allocate_zone(size, LARGE);
    _zone_push_back(zone, new_zone);
    pthread_mutex_unlock(&g_mutex);
    return (void*) new_zone + ALIGNED_ZONE_METADATA + CHUNK_HEADER_SIZE;
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

        remove_from_free_list(zone, it);

        // check if there is remaining space to split the freed chunk
        size_t remaining_size = raw_it_size - size;
        if (remaining_size < MIN_FREED_CHUNK_SIZE) {
            // update the next chunk in memory to remove IS_PREV_FREE flag
            chunk_header_t *fw_chunk = ADVANCE_CHUNK(it);
            fw_chunk->size &= ~(size_t)IS_PREV_FREE;
            if ((void*) fw_chunk > (void*) zone + zone->size)
                exit(1);
        } else {
            // update the size of the found chunk
            it->size = size;

            // create new freed chunk inside the remaining space area
            freed_header_t *new_chunk = ADVANCE_CHUNK(it);
            bzero(new_chunk, remaining_size); // reset metadata
            new_chunk->size = remaining_size;

            // create footer for backward coalescing
            WRITE_FOOTER(new_chunk);

            free_list_push_front(zone, new_chunk);
        }
        return (void*) it + CHUNK_HEADER_SIZE;
    }
    return NULL;
}

// `size` must be the aligned.
// If available, move the top chunk forward at the end of the user allocated
// chunk.
static void *_carve_space(zone_metadata_t *zone, size_t size) {
    if (
        (void*) zone->top + CHUNK_HEADER_SIZE + size >
        (void*) zone + zone->size
    ) {
        return NULL; // no space available in zone
    }

    // create new user space and set metadata
    chunk_header_t *allocated = zone->top;
    allocated->size = size;

    size_t old_size = zone->top->size;
    zone->top = (void*) zone->top + size;
    zone->top->size = old_size - size;

    return (void*) allocated + CHUNK_HEADER_SIZE;
}

// The `MAX` macro make sure that whatever the payload/size requested
// by the user is, it will always at least be MIN_FREED_CHUNK_SIZE
// to fit the freed chunk metadata once freed, therefore its range is:
// [MIN_FREED_CHUNK_SIZE, ALIGNED_PAYLOAD + ALIGNED_METADATA]
#define ALIGNED_MIN_CHUNK_SIZE(size) MAX(ALIGN(size) + CHUNK_HEADER_SIZE, \
    MIN_FREED_CHUNK_SIZE)

// Return a user space address, else `NULL` mean that there is no space left.
static void *_search_free_chunk(zone_metadata_t *zone, size_t size) {
    const size_t min_size = ALIGNED_MIN_CHUNK_SIZE(size);

    void *chunk;
    zone_metadata_t *zone_it;
    for (zone_it = zone; zone_it != NULL; zone_it = zone_it->next) {
        chunk = _search_through_free_list(zone_it, min_size);
        if (chunk)
            return chunk;
        chunk = _carve_space(zone_it, min_size);
        if (chunk)
            return chunk;
    }

    return NULL;
}

void *malloc(size_t size) {
    if (size == 0)
        return NULL;

    pthread_mutex_lock(&g_mutex);

    const struct zone_info_s inf = get_zone_infos(size);
    if (inf.type == LARGE)
        return _handle_large_alloc(inf.zone, size);

    void *chunk = _search_free_chunk(*inf.zone, size);
    if (chunk) {
        pthread_mutex_unlock(&g_mutex);
        return chunk;
    }

    zone_metadata_t *new_zone = _allocate_zone(size, inf.type);
    if (!new_zone) {
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }

    _zone_push_back(inf.zone, new_zone);
    chunk = _carve_space(new_zone, ALIGNED_MIN_CHUNK_SIZE(size));
    pthread_mutex_unlock(&g_mutex);
    return chunk;
}
