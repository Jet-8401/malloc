#include "libft_malloc.h"
#include <pthread.h>
#include <unistd.h>

static const size_t MIN_ALLOCS = 100;

#define HEADER_SIZE_VALUE ALIGN(sizeof(chunk_header_t))

mctx_t mctx = {
    .allocator = { NULL, NULL, NULL },

    .g_lock = PTHREAD_MUTEX_INITIALIZER,

    .ALIGNED_ZONE_METADATA = ALIGN(sizeof(zone_metadata_t)),
    // .ALIGNED_LARGE_ZONE_META = ALIGN(sizeof(large_zone_meta_t)),
    .HEADER_SIZE = HEADER_SIZE_VALUE,
    .MIN_CHUNK_SIZE = ALIGN(sizeof(freed_header_t) + sizeof(freed_footer_t)),

    .TINY_ZONE_ALLOC_SIZE =
        ALIGN((TINY_ZONE_TRESHOLD + HEADER_SIZE_VALUE)) * MIN_ALLOCS,
    .SMALL_ZONE_ALLOC_SIZE =
        ALIGN((SMALL_ZONE_TRESHOLD + HEADER_SIZE_VALUE)) * MIN_ALLOCS
};
