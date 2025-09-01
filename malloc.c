#include "libft_malloc.h"
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdalign.h>

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

    size += ALIGNED_METADATA_SIZE;
    *zone = mmap(NULL, size, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (*zone == MAP_FAILED)
        return -1;

    const size_t ps = sysconf(_SC_PAGESIZE);
    // Instead of just puting the argument `size` inside the list metadata
    // we calculate the size based on the page size because mmap will return
    // an address aligned to it therefore, it might allocate a lot more than
    // what was requested.
    // That way we account for the full allocated size.
    (*zone)->begin = ((void*) *zone) + ALIGNED_METADATA_SIZE;
    (*zone)->begin->size = ((size + ps - 1) / ps) * ps - ALIGNED_METADATA_SIZE;

    return 0;
}

// return an address that fit the size parameter or NULL if can't find one
static void *search_free_block_in_zone(
    zone_metadata_t *zone,
    const size_t size
) {
    freed_block_list_t *block_it;

    for (block_it = zone->begin; block_it != NULL; block_it = block_it->next) {
        if (block_it->size < size)
            continue;

        void *result = block_it;
        const size_t aligned_size = ALIGN_SIZE(size);
        const size_t remaining_size = block_it->size - aligned_size;

        if (remaining_size < sizeof(freed_block_list_t)) {
            zone->begin = block_it->next;
            continue;
        }

        freed_block_list_t *next_location = (void*) block_it + aligned_size;
        next_location->size = remaining_size;
        next_location->next = block_it->next;
        zone->begin = next_location;
        return result;
    }

    return NULL;
}

static void* search_freed_block(zone_metadata_t *zone, const size_t size) {
    zone_metadata_t *zone_it = zone;

    while (zone_it) {
        void* result = search_free_block_in_zone(zone_it, size);
        if (result)
            return result;
        zone_it = zone_it->next;
    }

    return NULL;
}

void *malloc(size_t size) {
    // get the zone type based on the size
    zone_metadata_t **zone = &g_allocator.large_zone;
    enum ZONE_TYPE zone_type = LARGE;
    if (size <= TINY_ZONE_TRESHOLD) {
        zone = &g_allocator.tiny_zone;
        zone_type = TINY;
    } else if (size <= SMALL_ZONE_TRESHOLD) {
        zone = &g_allocator.small_zone;
        zone_type = SMALL;
    }

    // check if zone point to NULL
    if (!*zone && allocate_zone(zone, size, zone_type) == -1) {
        return NULL;
    }

    void *block = search_freed_block(*zone, size);
    if (block)
        return block;

    zone_metadata_t *zone_it = *zone;
    while (zone_it->next != NULL)
        zone_it = zone_it->next;
    if (allocate_zone(&(zone_it->next), size, zone_type) == -1)
        return NULL;

    return search_freed_block(*zone, size);
}
