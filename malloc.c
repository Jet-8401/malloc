#include "libft_malloc.h"
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
/* ensure SIZE_MAX is available; fallback to portable expression if not */
#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

#define alloc(size) mmap(NULL, size, PROT_READ | PROT_WRITE, \
    MAP_ANON | MAP_PRIVATE, -1, 0)

static int _compute_chunk_size(size_t user_size, size_t *chunk_size) {
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

    // make chunk be at least MIN_FREED_CHUNK_SIZE to fit the freed chunk
    // metadata once freed
    size_t chunk = aligned + mctx.HEADER_SIZE;
    if (chunk < mctx.MIN_CHUNK_SIZE)
        chunk = mctx.MIN_CHUNK_SIZE;

    *chunk_size = chunk;
    return 0;
}

static void *_handle_large_alloc(size_t chunk_size) {
    const size_t size = ALIGN_TO(chunk_size, sysconf(_SC_PAGESIZE)) +
        mctx.ALIGNED_ZONE_METADATA;

    zone_metadata_t *allocated_zone = alloc(size);
    zone_push_back(&mctx.allocator.large_zone, allocated_zone);
    allocated_zone->size = size;

    chunk_header_t *chunk = (chunk_header_t*)
        ((uint8_t*) allocated_zone + mctx.ALIGNED_ZONE_METADATA);
    chunk->size = size - mctx.ALIGNED_ZONE_METADATA;

    return (uint8_t*) chunk + mctx.HEADER_SIZE;
}

static void remove_from_list(freed_header_t **origin, freed_header_t *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else if (node == *origin) {
        *origin = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }

    node->next = NULL;
    node->prev = NULL;
}

static void *_search_free_list(zone_metadata_t *zone, size_t chunk_size) {
    freed_header_t *chunk_it;
    for (
        chunk_it = zone->begin;
        chunk_it != NULL;
        chunk_it = chunk_it->next
    ) {
        if (chunk_it->size < chunk_size)
            continue;

        remove_from_list(&zone->begin, chunk_it);

        return (uint8_t*) chunk_it + mctx.HEADER_SIZE;
    }
    return NULL;
}

// Try to carve space from the top chunk by moving it by chunk_size.
// Return NULL if no space left.
static void *_carve_space(zone_metadata_t *zone, size_t chunk_size) {
    // check if we have at least HEADER_SIZE left for the top metadata
    if (zone->top->size - chunk_size < mctx.HEADER_SIZE)
        return NULL;

    chunk_header_t *allocated_chunk = zone->top;
    allocated_chunk->size = chunk_size;

    size_t new_size = zone->top->size - chunk_size;
    zone->top = (chunk_header_t*) ((uint8_t*) zone->top + chunk_size);
    zone->top->size = new_size;

    return (uint8_t*) allocated_chunk + mctx.HEADER_SIZE;
}

// Return a pointer to a new zone freshly mmaped or NULL if an error occur.
// Don't handle LARGE zone.
static zone_metadata_t* _alloc_zone(enum ZONE_TYPE type) {
    size_t size = mctx.ALIGNED_ZONE_METADATA;
    switch (type) {
        case TINY:
            size += mctx.TINY_ZONE_ALLOC_SIZE;
            break;
        case SMALL:
            size += mctx.SMALL_ZONE_ALLOC_SIZE;
            break;
        case LARGE:
            errno = EINVAL;
            return NULL;
    }

    // Align the space required to the page alignment.
    size = ALIGN_TO(size, sysconf(_SC_PAGESIZE));

    zone_metadata_t *zone = alloc(size);
    if (zone == MAP_FAILED) {
        return NULL;
    }

    memset(zone, 0, sizeof(zone_metadata_t));

    zone->size = size;
    zone->top = (chunk_header_t*)
        ((uint8_t*) zone + mctx.ALIGNED_ZONE_METADATA);
    zone->top->size = size - mctx.ALIGNED_ZONE_METADATA;

    return zone;
}

static void *_handle_alloc(struct zone_info_s info, size_t chunk_size) {
    void *chunk = NULL;
    zone_metadata_t *zone_it, *last_zone = NULL;

    for (
        zone_it = *info.zone;
        zone_it != NULL;
        last_zone = zone_it, zone_it = zone_it->next
    ) {
        chunk = _search_free_list(zone_it, chunk_size);
        if (chunk)
            return chunk;
        chunk = _carve_space(zone_it, chunk_size);
        if (chunk)
            return chunk;
    }

    // if we arrived to this point it mean that no space is left
    zone_metadata_t *allocated_zone = _alloc_zone(info.type);
    if (!allocated_zone)
        return NULL;

    // link the newly allocated zone
    if (last_zone != NULL) {
        allocated_zone->prev = last_zone;
        last_zone->next = allocated_zone;
    } else {
        allocated_zone->prev = NULL;
        *info.zone = allocated_zone;
    }

    return _carve_space(allocated_zone, chunk_size);
}

void *malloc(size_t size) {
    void *chunk;
    const struct zone_info_s info = get_zone_infos(size);

    size_t chunk_size;
    if (_compute_chunk_size(size, &chunk_size) == -1)
        return NULL;

    // lock the zone mutex inside malloc
    // and don't touch the mutex anywhere else
    pthread_mutex_lock(info.lock);

    if (info.type == LARGE) {
        chunk = _handle_large_alloc(chunk_size);
    } else {
        chunk = _handle_alloc(info, chunk_size);
    }

    pthread_mutex_unlock(info.lock);
    return chunk;
}
