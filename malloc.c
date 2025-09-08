#include "libft_malloc.h"
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdalign.h>

static const size_t TINY_ZONE_ALLOC = TINY_ZONE_TRESHOLD * 100;
static const size_t SMALL_ZONE_ALLOC = SMALL_ZONE_TRESHOLD * 100;
static const size_t ALIGNED_ZONE_METADATA = ALIGN(sizeof(zone_metadata_t));

static int allocate_zone(
    zone_metadata_t **zone,
    size_t size,
    const enum ZONE_TYPE type
) {
    switch (type) {
        case TINY:
            size = TINY_ZONE_ALLOC;
            break;
        case SMALL:
            size = SMALL_ZONE_ALLOC;
            break;
        default:
            break;
    }

    size += ALIGNED_ZONE_METADATA;
    *zone = mmap(
        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0
    );
    if (*zone == MAP_FAILED)
        return -1;

    const size_t ps = sysconf(_SC_PAGESIZE);
    const size_t full_alloc = ((size + ps - 1) / ps) * ps;
    // Instead of just puting the argument `size` inside the list metadata
    // we calculate the size based on the page size because mmap will return
    // an address aligned to it therefore, it might allocate a lot more than
    // what was requested.
    // That way we account for the full allocated size.
    (*zone)->begin = (void*) zone + ALIGNED_ZONE_METADATA;
    (*zone)->begin->size = full_alloc - ALIGNED_ZONE_METADATA;

    return 0;
}

// return an address that fit the size parameter or NULL if can't find one
static void *search_free_chunk_in_zone(
    zone_metadata_t *zone,
    const size_t size
) {
    freed_chunk_header_t *chunk_it;
    freed_chunk_header_t *prev_chunk = NULL;

    for (
        chunk_it = zone->begin;
        chunk_it != NULL;
        prev_chunk = chunk_it, chunk_it = chunk_it->next
    ) {
        // if the remaining size of not enough to fit the metadata of a freed
        // chunk, then move the pointer of the zone and don't return anything
        if (chunk_it->size < (size_t) MIN_FREED_CHUNK_SIZE) {
            zone->begin = chunk_it->next;
            return NULL;
        }

        size_t new_chunk_size = ALIGN(size + CHUNK_HEADER_SIZE);
        // size_t remaining_size = chunk_it->size - new_chunk_size;

        // setup metadata of the newly allocated chunk
        size_t flags = chunk_it->size & CHUNK_META_MASK;
        chunk_it->size = new_chunk_size | flags;
        // size_t full_size = MIN_CHUNK_SIZE + ALIGN(size);

        // calculate where to move the freed chunk
        freed_chunk_header_t *next_freed_chunk = ((void*)chunk_it) + new_chunk_size;
        next_freed_chunk->size &= ~IS_PREV_FREE;

        if (prev_chunk)
            prev_chunk->next = next_freed_chunk;
        else
            zone->begin = next_freed_chunk;

        // return the user payload from the allocated chunk
        return ((void*) chunk_it) + CHUNK_HEADER_SIZE;
    }

    return NULL;
}

static void* search_freed_chunk(zone_metadata_t *zone, const size_t size) {
    zone_metadata_t *zone_it = zone;

    while (zone_it) {
        void* result = search_free_chunk_in_zone(zone_it, size);
        if (result)
            return result;
        zone_it = zone_it->next;
    }

    return NULL;
}

void *malloc(size_t size) {
    struct zone_info_s infos = _get_zone_infos(size);

    if (!*infos.zone && allocate_zone(infos.zone, size, infos.type) == -1) {
        return NULL;
    }

    void *chunk = search_freed_chunk(*infos.zone, size);
    if (chunk)
        return chunk;

    zone_metadata_t *zone_it = *infos.zone;
    while (zone_it->next != NULL)
        zone_it = zone_it->next;
    if (allocate_zone(&(zone_it->next), size, infos.type) == -1)
        return NULL;

    return search_freed_chunk(*infos.zone, size);
}
