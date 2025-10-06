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

    zone->top = ((void*)zone) + ALIGNED_ZONE_METADATA;
    zone->top->size = size - ALIGNED_ZONE_METADATA;

    zone->begin = NULL;

    return zone;
}

// static bool split_remaining_memory(size_t remaining_size) {
//     if ()

//     freed_header_t *remainder;

//     // if the remaining size cannot fit a freed chunk's metadata
//     // then we don't split

//     if (remaining_size < MIN_FREED_CHUNK_SIZE) {
//         remainder = NULL;
//     } else {
//         remainder = (void*) it + UNMASK(alloc_chunk->size);
//         remainder->next = it->next;
//         remainder->prev = prev_chunk;
//         remainder->size = remaining_size | (it->size & CHUNK_META_MASK);

//         // update the footer of the freed chunk
//         size_t footer_offset = remainder->size - sizeof(freed_footer_t);
//         freed_footer_t *footer = (void*) remainder + footer_offset;
//         footer->prev_size = remainder->size;
//     }

//     if (remainder && remainder->next)
//         remainder->next->prev = remainder;

//     if (prev_chunk)
//         prev_chunk->next = remainder;
//     else
//         zone->begin = remainder;
// }

// return an address that fit the size parameter or NULL if can't find one
static void *search_free_chunk_in_zone(
    zone_metadata_t *zone,
    const size_t size
) {
    freed_header_t *it;
    freed_header_t *prev_chunk = NULL;
    const size_t min_size = max(
        ALIGN(size) + CHUNK_HEADER_SIZE, MIN_FREED_CHUNK_SIZE
    );

    for (
        it = zone->begin;
        it != NULL;
        prev_chunk = it, it = it->next
    ) {
        size_t raw_it_size = UNMASK(it->size);
        if (raw_it_size < min_size)
            continue;

        chunk_header_t *alloc_chunk = (void*) it;
        alloc_chunk->size = min_size;

        size_t remaining_size = raw_it_size - min_size;
        if (remaining_size < MIN_FREED_CHUNK_SIZE) {
            // update the double linked list
            if (prev_chunk) prev_chunk->next = it->next;
            if (it->next) it->next->prev = prev_chunk;
            if (zone->begin == it) zone->begin = it->next;

            // update the size of the allocated chunk
            alloc_chunk->size = raw_it_size;
        } else {
            // create a freed chunk in memory
            freed_header_t *remainder = (void*) it + alloc_chunk->size;
            remainder->size = remaining_size;
            remainder->prev = prev_chunk;
            remainder->next = it->next;

            // write the footer
            size_t footer_offset = remainder->size - sizeof(freed_footer_t);
            freed_footer_t *footer = (void*) remainder + footer_offset;
            footer->prev_size = remainder->size;

            // update the double linked list
            if (prev_chunk) prev_chunk->next = remainder;
            if (it->next) it->next->prev = remainder;
            if (zone->begin == it) zone->begin = remainder;
        }

        return ((void*) alloc_chunk) + CHUNK_HEADER_SIZE;
    }

    return NULL;
}

struct search_result_s {
    void            *chunk;
    zone_metadata_t *last_zone;
};

static struct search_result_s search_freed_chunk(
    zone_metadata_t *zone, const size_t size
) {
    zone_metadata_t *zone_it = zone;
    zone_metadata_t *last = zone;

    while (zone_it) {
        last = zone_it;
        void* result = search_free_chunk_in_zone(zone_it, size);
        if (result)
            return (struct search_result_s){ .chunk=result, .last_zone=last };
        zone_it = zone_it->next;
    }

    return (struct search_result_s){ .chunk=NULL, .last_zone=last };
}

static void zone_push_last(zone_metadata_t **src, zone_metadata_t *element) {
    if (*src == NULL) {
        *src = element;
        return;
    }

    zone_metadata_t *zone_it = *src;
    while (zone_it->next)
        zone_it = zone_it->next;
    zone_it->next = element;
}

static void* carve_from_top_chunk(zone_metadata_t *zone, size_t size) {
    size = max(ALIGN(size) + CHUNK_HEADER_SIZE, MIN_FREED_CHUNK_SIZE);

    const void* next_top = (void*) zone->top + size;
    if (next_top > (void*) zone + zone->size)
        return NULL; // Not enough space

    chunk_header_t* alloc_chunk = (void*) zone->top;
    const size_t inherited_flags = alloc_chunk->size & CHUNK_META_MASK;

    const size_t previous_zone_size = UNMASK(zone->top->size);
    zone->top = (void*) next_top;
    zone->top->size = previous_zone_size - size;

    alloc_chunk->size = size | inherited_flags;
    return (void*) alloc_chunk + CHUNK_HEADER_SIZE;
}

// Return NULL if not found any block.
static void *search_chunk(struct zone_info_s infos, size_t size) {
    // first search from the freed list
    struct search_result_s res = search_freed_chunk(*infos.zone, size);
    if (res.chunk)
        return res.chunk;

    // then if the exist carve some raw space for the allocated chunk
    if (res.last_zone) {
        void* chunk = carve_from_top_chunk(res.last_zone, size);
        if (chunk)
            return chunk;
    }

    return NULL;
}

void *malloc(size_t size) {
    if (size == 0)
        return NULL;

    struct zone_info_s infos = _get_zone_infos(size);

    void *chunk = NULL;
    if (infos.type != LARGE) {
        chunk = search_chunk(infos, size);
        if (chunk)
            return chunk;
    }

    zone_metadata_t *new_zone = allocate_zone(size, infos.type);
    if (!new_zone)
        return NULL;
    zone_push_last(infos.zone, new_zone);

    chunk = carve_from_top_chunk(new_zone, size);
    return chunk;
}
