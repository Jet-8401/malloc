#include "libft_malloc.h"

struct zone_info_s get_zone_infos(const size_t payload_size) {
    struct zone_info_s info;

    if (payload_size <= TINY_ZONE_TRESHOLD) {
        info.zone = &g_allocator.tiny_zone;
        info.type = TINY;
        info.lock = &g_allocator.tiny_lock;
    } else if (payload_size <= SMALL_ZONE_TRESHOLD) {
        info.zone = &g_allocator.small_zone;
        info.type = SMALL;
        info.lock = &g_allocator.small_lock;
    } else {
        info.zone = &g_allocator.large_zone;
        info.type = LARGE;
        info.lock = &g_allocator.large_lock;
    }

    return info;
}
