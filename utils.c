#include "libft_malloc.h"

struct zone_info_s get_zone_infos(const size_t payload_size) {
    zone_metadata_t **zone = &g_allocator.large_zone;
    enum ZONE_TYPE zone_type = LARGE;
    if (payload_size <= TINY_ZONE_TRESHOLD) {
        zone = &g_allocator.tiny_zone;
        zone_type = TINY;
    } else if (payload_size <= SMALL_ZONE_TRESHOLD) {
        zone = &g_allocator.small_zone;
        zone_type = SMALL;
    }

    return (struct zone_info_s){ .zone = zone, .type = zone_type };
}

void remove_from_free_list(zone_metadata_t *zone, freed_header_t *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        zone->begin = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
}

void free_list_push_front(zone_metadata_t *zone, freed_header_t *node) {
    node->next = zone->begin;
    node->prev = NULL;

    if (zone->begin) {
        zone->begin->prev = node;
    }

    zone->begin = node;
}

void    show_alloc_mem() {

}
