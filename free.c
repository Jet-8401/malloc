#include "libft_malloc.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/mman.h>

typedef enum {
    NONE,
    COALESCE_FORWARD = 1 << 0,
    COALESCE_BACKWARD = 1 << 1,
    TOP_CHUNK_ABSORB = 1 << 2
}   free_actions_e;

typedef void (*coalesce_strategy)(freed_header_t *chunk, zone_metadata_t *zone);

void coalesce_none(freed_header_t *chunk, zone_metadata_t *zone) {
    free_list_push_front(zone, chunk);

    chunk_header_t *fw_chunk = ADVANCE_CHUNK(chunk);
    fw_chunk->size |= IS_PREV_FREE;
    WRITE_FOOTER(chunk);
}

void coalesce_forward(freed_header_t *chunk, zone_metadata_t *zone) {
    freed_header_t *fw_chunk = ADVANCE_CHUNK(chunk);
    remove_from_free_list(zone, fw_chunk);

    chunk->size += UNMASK(fw_chunk->size);
    WRITE_FOOTER(chunk);

    free_list_push_front(zone, chunk);
}

void coalesce_backward(freed_header_t *chunk, zone_metadata_t *zone) {
    (void) zone;

    freed_footer_t *prev_footer = (void*) chunk - sizeof(freed_footer_t);
    freed_header_t *bw_chunk = (void*) chunk - prev_footer->prev_size;

    bw_chunk->size += UNMASK(chunk->size);
    WRITE_FOOTER(bw_chunk);
}

void top_chunk_absorb(freed_header_t *chunk, zone_metadata_t *zone) {
    size_t raw_size = UNMASK(chunk->size);

    size_t old_size = zone->top->size;
    zone->top = (void*) zone->top - raw_size;
    zone->top->size = old_size + raw_size;
}

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
coalesce_strategy strategies[] = {
    coalesce_none,
    coalesce_forward,
    coalesce_backward,
    top_chunk_absorb
};

zone_metadata_t *_get_subzone(zone_metadata_t *zone, void *ptr) {
    while (zone != NULL) {
        if (ptr > (void*) zone && ptr < (void*) zone + zone->size)
            return zone;
        zone = zone->next;
    }
    return NULL;
}

void _handle_large_free(
    zone_metadata_t **og_zone,
    freed_header_t *data
) {
    zone_metadata_t *zone;
    zone_metadata_t *prev = NULL;
    void *ptr = data;

    for (zone = *og_zone; zone != NULL; prev = zone, zone = zone->next) {
        if (ptr >= (void*) zone && ptr < (void*) zone + zone->size) {
            // Found the zone, now remove it from the linked list
            if (prev == NULL) *og_zone = zone->next; // This is the first zone in the list
            else prev->next = zone->next; // This is not the first zone

            munmap(zone, zone->size);
            zone = NULL;
            pthread_mutex_unlock(&g_mutex);
            return;
        }
    }

    // If we get here, the pointer wasn't found in any zone
    pthread_mutex_unlock(&g_mutex);
}

void free(void *ptr) {
    if (!ptr)
        return;

    pthread_mutex_lock(&g_mutex);

    freed_header_t *data = (void*) ptr - CHUNK_HEADER_SIZE;
    const size_t payload_size = UNMASK(data->size) - CHUNK_HEADER_SIZE;
    const struct zone_info_s inf = get_zone_infos(payload_size);
    if (inf.type == LARGE)
        return _handle_large_free(inf.zone, data);
    zone_metadata_t *subzone = _get_subzone(*inf.zone, data);
    if (!subzone) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    uint8_t actions = NONE;

    if (data->size & IS_PREV_FREE) {
        actions |= COALESCE_BACKWARD;
    }

    chunk_header_t *fw_chunk = ADVANCE_CHUNK(data);
    if (fw_chunk != subzone->top) {
        fw_chunk = ADVANCE_CHUNK(fw_chunk);
        if (fw_chunk->size & IS_PREV_FREE)
            actions |= COALESCE_FORWARD;
    } else {
        actions |= TOP_CHUNK_ABSORB;
    }

    if (actions == NONE) coalesce_none(data, subzone);
    if (actions & COALESCE_FORWARD) coalesce_forward(data, subzone);
    if (actions & COALESCE_BACKWARD) coalesce_backward(data, subzone);
    if (actions & TOP_CHUNK_ABSORB) top_chunk_absorb(data, subzone);

    // strategies[actions](data, subzone);

    pthread_mutex_unlock(&g_mutex);
}
