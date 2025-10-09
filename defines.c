#include "libft_malloc.h"
#include <unistd.h>

allocator_t g_allocator = { NULL, NULL, NULL };
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Global defines */
const unsigned char MEM_ALIGNMENT = _Alignof(max_align_t) <= 8 ?
    8 : _Alignof(max_align_t);

/* Memory defines */
const unsigned char ALIGNED_ZONE_METADATA = ALIGN(sizeof(zone_metadata_t));

/* Chunks defines */
const unsigned char CHUNK_HEADER_SIZE = ALIGN(sizeof(chunk_header_t));
const unsigned char MIN_FREED_CHUNK_SIZE = ALIGN(
    sizeof(freed_header_t) + sizeof(freed_footer_t)
);

/* Zones defines */
static const size_t MIN_ALLOCS = 100;
const size_t TINY_ZONE_ALLOC_SIZE =
    ALIGN((TINY_ZONE_TRESHOLD + CHUNK_HEADER_SIZE)) * MIN_ALLOCS;
const size_t SMALL_ZONE_ALLOC_SIZE =
    ALIGN((SMALL_ZONE_TRESHOLD + CHUNK_HEADER_SIZE)) * MIN_ALLOCS;
