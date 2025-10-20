#include "libft_malloc.h"

struct zone_info_s get_zone_infos(const size_t payload_size) {
    struct zone_info_s info;

    if (payload_size <= TINY_ZONE_TRESHOLD) {
        info.zone = &mctx.allocator.tiny_zone;
        info.type = TINY;
        info.lock = &mctx.tiny_lock;
    } else if (payload_size <= SMALL_ZONE_TRESHOLD) {
        info.zone = &mctx.allocator.small_zone;
        info.type = SMALL;
        info.lock = &mctx.small_lock;
    } else {
        info.zone = &mctx.allocator.large_zone;
        info.type = LARGE;
        info.lock = &mctx.large_lock;
    }

    return info;
}

void zone_push_back(zone_metadata_t **head, zone_metadata_t *zone) {
    zone->next = NULL;

    if (*head == NULL) {
        *head = zone;
    } else {
        zone_metadata_t *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = zone;
    }
}
