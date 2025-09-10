#include "libft_malloc.h"
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdalign.h>

static const size_t TINY_ZONE_ALLOC = TINY_ZONE_TRESHOLD * 100;
static const size_t SMALL_ZONE_ALLOC = SMALL_ZONE_TRESHOLD * 100;

/* Allocate new zone and return its address.
 * Return NULL in case of error and set errno.
 */
static zone_metadata_t* allocate_zone(
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
        case LARGE:
            size += CHUNK_HEADER_SIZE;
            break;
        default:
            break;
    }

    zone_metadata_t *zone;

    size += ALIGNED_ZONE_METADATA;
    zone = mmap(
        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0
    );
    if (zone == MAP_FAILED)
        return NULL;

    const size_t ps = sysconf(_SC_PAGESIZE);
    const size_t full_alloc = ((size + ps - 1) / ps) * ps;
    // Instead of just puting the argument `size` inside the list metadata
    // we calculate the size based on the page size because mmap will return
    // an address aligned to it therefore, it might allocate a lot more than
    // what was requested.
    // That way we account for the full allocated size.
    zone->size = full_alloc;
    zone->next = NULL;

    zone->begin = ((void*)zone) + ALIGNED_ZONE_METADATA;
    zone->begin->size = full_alloc - ALIGNED_ZONE_METADATA;
    zone->begin->next = NULL;

    return zone;
}

// return an address that fit the size parameter or NULL if can't find one
static void *search_free_chunk_in_zone(
    zone_metadata_t *zone,
    const size_t size
) {
    freed_chunk_header_t *it;
    freed_chunk_header_t *prev_chunk = NULL;
    const size_t min_size = ALIGN(size) + CHUNK_HEADER_SIZE;

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

        freed_chunk_header_t *remainer;
        if (remaining_size < MIN_FREED_CHUNK_SIZE) {
            alloc_chunk->size = it->size;
            remainer = NULL;
        } else {
            remainer = ((void*)it) + alloc_chunk->size;
            remainer->next = it->next;
            remainer->size = remaining_size;
        }

        if (prev_chunk)
            prev_chunk->next = remainer;
        else
            zone->begin = remainer;

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

    void *chunk = search_freed_chunk(*infos.zone, size);
    if (chunk)
        return chunk;

    zone_metadata_t *new_zone = allocate_zone(size, infos.type);
    if (!new_zone)
        return NULL;
    zone_push_last(infos.zone, new_zone);

    void *value = search_free_chunk_in_zone(new_zone, size);
    return value;
}
