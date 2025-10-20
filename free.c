#include "libft_malloc.h"
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

static void _handle_large_free(zone_metadata_t **head, chunk_header_t *chunk) {
    zone_metadata_t *zone = (zone_metadata_t*)
        ((uint8_t*) chunk - mctx.ALIGNED_ZONE_METADATA);

    if (zone->prev == NULL && *head == zone) {
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
    } else {
        write(1, "yep\n", 4);
    }
}

static void _handle_free(chunk_header_t *chunk) {
    (void) chunk;
}

void free(void *ptr) {
    chunk_header_t *meta = (void*) ((uint8_t*) ptr - mctx.HEADER_SIZE);

    const struct zone_info_s info = get_zone_infos(GET_RAW_SIZE(meta));

    pthread_mutex_lock(info.lock);

    if (info.type == LARGE)
        _handle_large_free(info.zone, meta);
    else
        _handle_free(meta);

    pthread_mutex_unlock(info.lock);
}
