#include "libft_malloc.h"

struct zone_info_s  _get_zone_infos(const size_t payload_size) {
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

void    show_alloc_mem() {

}
