#include "libft_malloc.h"

// Search ptr inside a subzone.
// A subzone is a zone inside a linked list of zones.
static zone_metadata_t *get_subzone(zone_metadata_t *zone, void *ptr) {
    while (zone) {
        if (ptr > (void*) zone && ptr < (void*) zone + zone->size)
            return zone;
        zone = zone->next;
    }
    return NULL;
}

// Scenarios of free:
// 1. Can't coalesce, append new freed chunk inside zone list.
// 2. Can coalesce with previous block in zone so update the already
//    existing freed chunk's size.
// 3. Can coalesce with forward block in zone so remove it from the free list
//    and update the size of size.
//
// Note: 2 & 3 can co-exist at the same time, therefore we can remove some freed
// chunk and update the existing one.
// expl: [32 freed][32 allocated][32 freed]
// here we need to check the next chunk, if its free we need to add that to
// the total size that will be updated.

// Note:
//  - When freeing a point we need to check for the chunks surrounding it
//    inside memory to coalesce it if needed.
//  - If a chunk is freed right next to the "Top Chunk" it will absord it
//    and expand its size rather than creating an other freed chunk and append
//    it to the free list.
void free(void *ptr) {
    if (ptr == NULL)
        return;

    chunk_header_t *meta = ptr - CHUNK_HEADER_SIZE;
    struct zone_info_s infos = _get_zone_infos(
        (meta->size & ~CHUNK_META_MASK) - CHUNK_HEADER_SIZE // payload size
    );

    // to start we initialize the range to free to coalesce free chunks
    // with the range (range_begin, range_end]

    chunk_header_t *range_begin = meta;
    while (range_begin->size & IS_PREV_FREE) {
        // get the previous chunk size from the footer
        size_t prev_size = *(size_t*)((void*)range_begin -
            sizeof(freed_chunk_footer_t)) & ~CHUNK_META_MASK;
        range_begin = ((void*) range_begin) - prev_size;
    }

    chunk_header_t *range_end = (void*) meta + (meta->size & ~CHUNK_META_MASK);
    chunk_header_t *next_p = range_end;
    // for forward coalescing we need to take two chunks ahead for checking
    // with the "Top Chunk"
    while (next_p != (*infos.zone)->top) {
        // save the n+1 chunk size
        size_t prev_size = (next_p->size & ~CHUNK_META_MASK);
        // move next_p to n+2
        next_p = (void*) next_p + prev_size;
        // check the flags of the n+2 chunk to know if the n+1 is free
        if (!(next_p->size & IS_PREV_FREE))
            break;
        range_end = (void*) next_p;
    }

    // then we can establish in which scenarios we are to check if we need to
    // access the zone metadata

    zone_metadata_t *zone = get_subzone(*infos.zone, meta);
    if (zone)
        return;

    // here ptr is inside the `zone` var
    // we don't always need to get the zone that way since in scenario 2 & 3
    // we don't need to append anything to the free list

    return;
}

// Top Chunk strategy:
// We need to create a dynamic boudary tags called "Top Chunk" for forward
// coalescing, this is because backwards coalescing is already handled via
// the LSB of the size meta data since the alignment is minimum of 8 bytes.
