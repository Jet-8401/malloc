#include "libft_malloc.h"
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdalign.h>

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

// static const size_t TINY_ZONE_ALLOC = (TINY_ZONE_TRESHOLD + CHUNK_HEADER_SIZE) * MIN_ALLOCS;

// static const size_t MIN_ALLOCS = 100;
// static const size_t TINY_ZONE_ALLOC = TINY_ZONE_TRESHOLD * MIN_ALLOCS;
// static const size_t SMALL_ZONE_ALLOC = SMALL_ZONE_TRESHOLD * MIN_ALLOCS;

/* Allocate new zone and return its address.
 * Return NULL in case of error and set errno.
 */
static zone_metadata_t* allocate_zone(
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

    // Aligning the raw minimum size required to the page size alignment.
    size = ALIGN_TO(size, sysconf(_SC_PAGESIZE));

    zone_metadata_t *zone = mmap(
        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0
    );
    if (zone == MAP_FAILED)
        return NULL;

    zone->size = size;
    zone->next = NULL;

    zone->begin = ((void*)zone) + ALIGNED_ZONE_METADATA;
    zone->begin->size = size - ALIGNED_ZONE_METADATA;
    zone->begin->next = NULL;

    return zone;
}

// return an address that fit the size parameter or NULL if can't find one
// TODO:  check that the chunk size allocated is >= to MIN_FREED_CHUNK_SIZE
static void *search_free_chunk_in_zone(
    zone_metadata_t *zone,
    const size_t size
) {
    freed_chunk_header_t *it;
    freed_chunk_header_t *prev_chunk = NULL;
    const size_t min_size = max(
        ALIGN(size) + CHUNK_HEADER_SIZE, MIN_FREED_CHUNK_SIZE
    );

    for (
        it = zone->begin;
        it != NULL;
        prev_chunk = it, it = it->next
    ) {
        if (it->size < min_size)
            continue;

        size_t remaining_size = it->size - min_size;

        chunk_header_t *alloc_chunk = (void*) it;
        alloc_chunk->size = min_size;

        freed_chunk_header_t *remainder;
        // if the remaining size cannot fit a freed chunk's metadata
        // then we don't split
        if (remaining_size < MIN_FREED_CHUNK_SIZE) {
            alloc_chunk->size = it->size;
            remainder = NULL;
        } else {
            remainder = ((void*)it) + alloc_chunk->size;
            remainder->next = it->next;
            remainder->size = remaining_size;
        }

        if (prev_chunk)
            prev_chunk->next = remainder;
        else
            zone->begin = remainder;

        return ((void*)alloc_chunk) + CHUNK_HEADER_SIZE;
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

void zone_push_last(zone_metadata_t **src, zone_metadata_t *element) {
    if (*src == NULL) {
        *src = element;
        return;
    }

    zone_metadata_t *zone_it = *src;
    while (zone_it->next)
        zone_it = zone_it->next;
    zone_it->next = element;
}

void *malloc(size_t size) {
    if (size == 0)
        return NULL;

    struct zone_info_s infos = _get_zone_infos(size);

    if (infos.type != LARGE) {
        void *chunk = search_freed_chunk(*infos.zone, size);
        if (chunk)
            return chunk;
    }

    zone_metadata_t *new_zone = allocate_zone(size, infos.type);
    if (!new_zone)
        return NULL;
    zone_push_last(infos.zone, new_zone);

    return search_free_chunk_in_zone(new_zone, size);
}
