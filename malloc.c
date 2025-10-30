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

static void _zone_push_back(zone_metadata_t **head, zone_metadata_t *zone) {
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

// Large allocations are direct mmap'ed and push to the back the list.
static void *_handle_large_alloc(size_t chunk_size) {
    const size_t page_size = sysconf(_SC_PAGESIZE);

    // check if ALIGN_TO would cause an overflow
    if (__builtin_expect(chunk_size > SIZE_MAX - page_size + 1, 0)) {
        errno = ENOMEM;
        return NULL;
    }

    const size_t aligned = ALIGN_TO(chunk_size, page_size);

    // check if adding metadata would overflow
    if (__builtin_expect(aligned > SIZE_MAX - mctx.ALIGNED_ZONE_METADATA, 0)) {
        errno = ENOMEM;
        return NULL;
    }

    const size_t size = aligned + mctx.ALIGNED_ZONE_METADATA;
    zone_metadata_t *allocated_zone = alloc(size);
    if (allocated_zone == MAP_FAILED)
        return NULL;

    allocated_zone->size = size;
    allocated_zone->type = LARGE;
    _zone_push_back(&mctx.allocator.large_zone, allocated_zone);

    chunk_header_t *chunk = (chunk_header_t*)
        ((uint8_t*) allocated_zone + mctx.ALIGNED_ZONE_METADATA);
    chunk->size = size - mctx.ALIGNED_ZONE_METADATA;

    return (uint8_t*) chunk + mctx.HEADER_SIZE;
}

static void *_search_free_list(zone_metadata_t *zone, size_t chunk_size) {
    freed_header_t *chunk_it;
    for (
        chunk_it = zone->begin;
        chunk_it != NULL;
        chunk_it = chunk_it->next
    ) {
        if (GET_RAW_SIZE(chunk_it) < chunk_size)
            continue;

        chunk_header_t *allocated_chunk = NULL;

        // check if we can split the freed chunk
        const size_t remaining_size = GET_RAW_SIZE(chunk_it) - chunk_size;
        if (remaining_size < mctx.MIN_CHUNK_SIZE) {
            // can't split because the remaining size of the splitted block
            // would not be enough to fit the minimum required space for a chunk

            remove_from_list(&zone->begin, chunk_it);

            // allocated_chunk become the whole freed chunk so no need to change
            // or update any values
            allocated_chunk = (chunk_header_t*) chunk_it;
        } else {
            // else the allocated chunk is taken after the freed memory so
            // we only need to update the freed chunk size
            // note: important to update the state of chunks such as recreating
            // the footer of the freed chunk and do MARK_FREE on chunk_it
            // to set the flags appropriately
            chunk_it->size = remaining_size;
            WRITE_FOOTER(chunk_it);

            // setup values of allocated chunk in memory
            allocated_chunk = ADVANCE_CHUNK((chunk_header_t*) chunk_it);
            allocated_chunk->size = chunk_size;

            MARK_FREE((chunk_header_t*) chunk_it);
        }

        MARK_ALLOCATED(allocated_chunk);

        // return the user space address
        return (uint8_t*) allocated_chunk + mctx.HEADER_SIZE;
    }
    return NULL;
}

// Try to carve space from the top chunk by moving it by chunk_size.
// Return NULL if no space left.
static void *_carve_space(zone_metadata_t *zone, size_t chunk_size) {
    // check if there is space available making sure the new chunk will not
    // overflow the current top chunk's metadata
    // note: both chunk size and top size are unsigned, make sure no overflow
    // can happend
    if (zone->top->size <= chunk_size ||
        zone->top->size - chunk_size <= mctx.HEADER_SIZE)
        return NULL;

    // save the top address
    void *prev_top = zone->top;

    // update the top chunk
    size_t new_size = zone->top->size - chunk_size;
    zone->top = (chunk_header_t*) ((uint8_t*) zone->top + chunk_size);
    zone->top->size = new_size;

    // restore the previous top address to the current allocated block
    chunk_header_t *allocated_chunk = prev_top;
    allocated_chunk->size = chunk_size;
    MARK_ALLOCATED(allocated_chunk);

    // return the user space address
    return (uint8_t*) allocated_chunk + mctx.HEADER_SIZE;
}

// Return a pointer to a new zone freshly mmaped or NULL if an error occur.
// Don't handle LARGE zone.
static zone_metadata_t *_alloc_zone(enum ZONE_TYPE type) {
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
    zone->type = type;
    zone->top = (chunk_header_t*)
        ((uint8_t*) zone + mctx.ALIGNED_ZONE_METADATA);
    zone->top->size = size - mctx.ALIGNED_ZONE_METADATA;

    return zone;
}

struct zone_info_s {
	zone_metadata_t **zone;
	enum ZONE_TYPE type;
};

// Handle the allocation of chunks with a first-fit algorithm.
// If no space is available the function will try to allocate more memory.
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

    // if we arrived to this point it means that no space is left
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

// Get the zone type and head of list based on the size of the payload
static struct zone_info_s _get_zone_infos(const size_t payload_size) {
    struct zone_info_s info;

    if (payload_size <= TINY_ZONE_TRESHOLD) {
        info.zone = &mctx.allocator.tiny_zone;
        info.type = TINY;
    } else if (payload_size <= SMALL_ZONE_TRESHOLD) {
        info.zone = &mctx.allocator.small_zone;
        info.type = SMALL;
    } else {
        info.zone = &mctx.allocator.large_zone;
        info.type = LARGE;
    }

    return info;
}

void *malloc(size_t size) {
    const struct zone_info_s info = _get_zone_infos(size);

    size_t chunk_size;
    if (compute_chunk_size(size, &chunk_size) == -1) {
        return NULL;
    }

    pthread_mutex_lock(&mctx.g_lock);

    void *chunk;
    if (info.type == LARGE) {
        chunk = _handle_large_alloc(chunk_size);
    } else {
        chunk = _handle_alloc(info, chunk_size);
    }

    pthread_mutex_unlock(&mctx.g_lock);
    return chunk;
}
