#include "libft_malloc.h"
#include <pthread.h>

void free(void *ptr) {
    if (!ptr)
        return;

    pthread_mutex_lock(&g_mutex);

    freed_header_t *data = (void*) ptr - CHUNK_HEADER_SIZE;
    struct zone_info_s inf = get_zone_infos(data->size - CHUNK_HEADER_SIZE);
}
