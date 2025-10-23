#include "libft_malloc.h"
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

static void _handle_large_free(zone_metadata_t **head, chunk_header_t *chunk) {
    zone_metadata_t *zone = (zone_metadata_t*)
        ((uint8_t*) chunk - mctx.ALIGNED_ZONE_METADATA);

    if (zone->prev == NULL) {
        *head = zone->next;
    } else {
        zone->prev->next = zone->next;
    }

    if (zone->next) {
        zone->next->prev = zone->prev;
    }

    zone->next = NULL;
    zone->prev = NULL;

    if (munmap(zone, zone->size) == -1) {
        write(2, "unmap error\n", 12);
    }
}

#define COALESCE_NONE 0
#define COALESCE_BACKWARD (1 << 0)
#define COALESCE_FORWARD (1 << 1)
#define TOP_CHUNK_ABSORB (1 << 2)

typedef void (*coalesce_strategy)(freed_header_t *chunk, zone_metadata_t *zone);

static void _free_list_push_front(zone_metadata_t *zone, freed_header_t *node) {
    node->next = zone->begin;
    node->prev = NULL;

    if (zone->begin) {
        zone->begin->prev = node;
    }
    zone->begin = node;
}

static void coalesce_none(freed_header_t *chunk, zone_metadata_t *zone) {
    chunk_header_t *fw_chunk = ADVANCE_CHUNK((chunk_header_t*) chunk);
    fw_chunk->size |= IS_PREV_FREE;

    _free_list_push_front(zone, chunk);
}

// static void null_function(void* chunk, void* zone) {
//     (void) chunk;
//     (void) zone;
// }

// COALESCE_NONE = adding free chunk to list
// COALESCE_BACKWARD = updating the backward chunk already in free list
// COALESCE_FORWARD = updating the free chunk inside the free list
// by moving its position inside the heap and its size to the total
// of the two chunk size
// TOP_CHUNK_ABSORB = only update the top chunk position
// COALESCE_BACKWARD & COALESCE_FORWARD = update the backward chunk in the
// free list by adding to to it the size of the current chunk size and the
// forward chunk size and remove the forward chunk from the free list
// COALESCE_BACKWARD & TOP_CHUNK_ABSORB = remove every references from free
// list and update the position of the top chunk
static coalesce_strategy strategies[] = {
    coalesce_none,          // 0b00000000
    // coalesce_forward,       // 0b00000001
    // coalesce_backward,      // 0b00000010
    // coalesce_both,          // 0b00000011
    // top_chunk_absorb,       // 0b00000100
    // null_function,          // 0b00000101
    // coalesce_bw_and_absorb, // 0b00000110
    // null_function           // 0b00000111
};

static void _handle_free(chunk_header_t *chunk, zone_metadata_t *zone) {
    uint8_t actions = COALESCE_NONE;

    // chunk_header_t *fw_chunk = ADVANCE_CHUNK(chunk);
    // if (fw_chunk != zone->top && !(fw_chunk->size & IS_ALLOCATED)) {
    //     actions |= COALESCE_FORWARD;
    // } else {
    //     actions |= TOP_CHUNK_ABSORB;
    // }

    // if (chunk->size & IS_PREV_FREE) {
    //     actions |= COALESCE_BACKWARD;
    // }

    MARK_FREE(chunk);
    freed_header_t *freed_chunk = (void*) chunk;
    // make sure to erase the metadata
    freed_chunk->next = NULL;
    freed_chunk->prev = NULL;

    strategies[actions](freed_chunk, zone);
}

// Return a boolean if the pointer have been found inside a zone.
// **zone_found can be null else it will put the zone where the pointer
// was found in it.
static bool _is_pointer_valid(
    zone_metadata_t *zone,
    void *ptr,
    zone_metadata_t **zone_found
) {
    for (; zone != NULL; zone = zone->next) {
        void *zone_payload = (uint8_t*) zone + mctx.ALIGNED_ZONE_METADATA;
        void *zone_end = (uint8_t*) zone + zone->size - mctx.HEADER_SIZE;
        if (ptr < zone_payload || ptr >= zone_end)
            continue;

        if (zone_found)
            *zone_found = zone;
        return true;
    }
    return false;
}

void free(void *ptr) {
    if (!ptr)
        return;

    zone_metadata_t *zone = NULL;
    // Range based validation for pointer ownsership, very slow and ineficient
    // but only way that won't take ages to implement and is 100% sure that
    // this pointer come from our malloc.
    // This is needed to test implementation on heavy program like /bin/bash.
    pthread_mutex_lock(&mctx.tiny_lock);
    if (!_is_pointer_valid(mctx.allocator.tiny_zone, ptr, &zone)) {
        pthread_mutex_unlock(&mctx.tiny_lock);
        return;
    }
    pthread_mutex_unlock(&mctx.tiny_lock);

    pthread_mutex_lock(&mctx.small_lock);
    if (!_is_pointer_valid(mctx.allocator.small_zone, ptr, &zone)) {
        pthread_mutex_unlock(&mctx.small_lock);
        return;
    }
    pthread_mutex_unlock(&mctx.small_lock);

    pthread_mutex_lock(&mctx.large_lock);
    if (!_is_pointer_valid(mctx.allocator.large_zone, ptr, &zone)) {
        pthread_mutex_unlock(&mctx.large_lock);
        return;
    }
    pthread_mutex_unlock(&mctx.large_lock);

    chunk_header_t *meta = (void*) ((uint8_t*) ptr - mctx.HEADER_SIZE);
    const struct zone_info_s info = get_zone_infos(
        GET_RAW_SIZE(meta) - mctx.HEADER_SIZE
    );

    pthread_mutex_lock(info.lock);

    if (info.type == LARGE)
        _handle_large_free(info.zone, meta);
    else
        _handle_free(meta, zone);

    pthread_mutex_unlock(info.lock);
}
