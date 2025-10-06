#include "libft_malloc.h"
#include <stdbool.h>
#include <stdint.h>

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

#define COALESCE_NONE 0
#define COALESCE_FORWARD 1 << 0
#define COALESCE_BACKWARD 1 << 1
#define TOP_CHUNK_ABSORB 1 << 2

// typedef void (*coalesce_strategy)(freed_header_t *chunk, zone_metadata_t *zone);

// void coalesce_none(freed_header_t *chunk, zone_metadata_t *zone) {

// }

// coalesce_strategy strategies[] = {
//     coalesce_none       // COALESCE_NONE
// };


// TODO: check if a union could be usefull

static void _remove_chunk_from_freelist(
    freed_header_t *chunk,
    zone_metadata_t *zone
) {
    if (chunk->prev) {
        chunk->prev->next = chunk->next;
    } else {
        zone->begin = chunk->next;
    }

    if (chunk->next) {
        chunk->next->prev = chunk->prev;
    }

    chunk->next = NULL;
    chunk->prev = NULL;
}

void free(void* ptr) {
    if (ptr == NULL)
        return;

    freed_header_t *meta = ptr - CHUNK_HEADER_SIZE;
    meta->next = NULL;
    meta->prev = NULL;

    struct zone_info_s inf = _get_zone_infos(
        UNMASK(meta->size) - CHUNK_HEADER_SIZE // payload size
    );
    zone_metadata_t *zone = get_subzone(*inf.zone, ptr);
    if (!zone)
        return;

    // COALESCE_NONE = adding free chunk to list
    // COALESCE_BACKWARD = updating the backward chunk already in free list
    // COALESCE_FORWARD = updating the free chunk inside the free list
    // by moving its position inside the heap and its size to the total
    // of the two chunk size
    // COALESCE_BACKWARD & COALESCE_FORWARD = update the backward chunk in the
    // free list by adding to to it the size of the current chunk size and the
    // forward chunk size and remove the forward chunk from the free list
    // COALESCE_BACKWARD & TOP_CHUNK_ABSORB = remove every references from free
    // list and update the position of the top chunk
    // COALESCE_FORWARD & TOP_CHUNK_ABSORB = only update the top chunk position

    uint8_t actions = COALESCE_NONE;

    if (meta->size & IS_PREV_FREE)
        actions |= COALESCE_BACKWARD;

    // note: `fw_chunk` is used to calculate forward coalescing later in code
    chunk_header_t *fw_chunk = (void*) meta + UNMASK(meta->size);
    if (fw_chunk != zone->top) {
        chunk_header_t *it = (void*) fw_chunk + UNMASK(fw_chunk->size);
        if (it->size & IS_PREV_FREE) {
            actions |= COALESCE_FORWARD;
        }
    } else {
        actions |= TOP_CHUNK_ABSORB;
    }

    // strategies[actions](meta, zone);

    freed_header_t *target = meta;
    size_t final_size = UNMASK(target->size);
    bool coalesced_backward = false;

    if (actions & COALESCE_BACKWARD) {
        freed_footer_t *prev_footer =
            (void*) target - sizeof(freed_footer_t);
        freed_header_t *prev_chunk =
            (void*) target - UNMASK(prev_footer->prev_size);
        final_size += UNMASK(prev_chunk->size);

        target = prev_chunk;
        coalesced_backward = true;
    }

    if (actions & COALESCE_FORWARD) {
        final_size += UNMASK(fw_chunk->size);

        _remove_chunk_from_freelist((freed_header_t*) fw_chunk, zone);
    }

    if (actions & TOP_CHUNK_ABSORB) {

        if (coalesced_backward) {
            _remove_chunk_from_freelist(target, zone);
        }

        size_t old_size = zone->top->size;
        zone->top = (void*) target;
        zone->top->size = old_size + final_size;
        return;
    }

    target->size = final_size | (target->size & CHUNK_META_MASK);
    freed_footer_t *footer =
        (void*) target + final_size - sizeof(freed_footer_t);
    footer->prev_size = target->size;

    // update the IS_PREV_FREE flag for the next chunk in memory
    fw_chunk = (void*) target + final_size;
    fw_chunk->size |= IS_PREV_FREE;

    // if we did not coalesced backward then we need to add the original
    // chunk inside the free list
    if (!coalesced_backward) {
        target->prev = NULL;
        target->next = zone->begin;
        if (target->next) {
            target->next->prev = target;
        }
        zone->begin = target;
    }
    return;
}

// Top Chunk strategy:
// We need to create a dynamic boudary tags called "Top Chunk" for forward
// coalescing, this is because backwards coalescing is already handled via
// the LSB of the size metadata since the alignment is minimum of 8 bytes.
