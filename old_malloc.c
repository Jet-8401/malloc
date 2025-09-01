#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdalign.h>
#include <unistd.h>
#include "libft_malloc.h"

static allocator_t g_allocator = { NULL, NULL, NULL };

int allocate_zone(
    zone_metadata_t **zone,
    size_t size,
    enum ZONE_TYPE zone_type
) {
    // for large zone default size would already be ideal so we don't handle it
    switch (zone_type) {
        case TINY:
            size = TINY_ZONE_ALLOC;
            break;
        case SMALL:
            size = SMALL_ZONE_TRESHOLD;
            break;
        default:
            break;
    }

    size += ALIGNED_METADATA_SIZE;
    *zone = mmap(NULL, size, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (*zone == MAP_FAILED)
        return -1;

    // insert zone metadata
    zone_metadata_t metadata = {
        .next = 0,
        .begin = (freed_block_list_t*)(
            ((char*) *zone) + ALIGNED_METADATA_SIZE
        )
    };
    memcpy(*zone, &metadata, sizeof(metadata));

    // insert list metadata
    const size_t ps = sysconf(_SC_PAGESIZE);
    freed_block_list_t block_metadata = {
        .next = 0,
        .size = (((size + ps - 1) / ps) * ps) - ALIGNED_METADATA_SIZE
    };
    memcpy((*zone)->begin, &block_metadata, sizeof(freed_block_list_t));
    return 0;
}

void *search_freed_block(zone_metadata_t **zone, size_t size) {
    freed_block_list_t *it = (*zone)->begin;
    freed_block_list_t *result = NULL;

    if (it->size >= size) {
        result = it;
        return result;
    }
    return NULL;
}

void *malloc(size_t size) {
    zone_metadata_t* zone_addr = g_allocator.large_zone;
    enum ZONE_TYPE zone_type = LARGE;

    if (size <= TINY_ZONE_TRESHOLD) {
        zone_addr = g_allocator.tiny_zone;
        zone_type = TINY;
    } else if (size <= SMALL_ZONE_TRESHOLD) {
        zone_addr = g_allocator.small_zone;
        zone_type = SMALL;
    }

    if (!zone_addr && allocate_zone(&zone_addr, size, zone_type) == -1) {
        return NULL;
    }

    // search a free block large enough
    return search_freed_block(&zone_addr, size);
}
