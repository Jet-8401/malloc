#include "libft_malloc.h"
#include <errno.h>
/* ensure SIZE_MAX is available; fallback to portable expression if not */
#include <stddef.h>
#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

static int _compute_chunk_size(size_t user_size, size_t *chunk_size) {
    // check ALIGN won't overflow
    if (user_size > SIZE_MAX - (MEM_ALIGNMENT - 1)) {
        errno = ENOMEM;
        return -1;
    }

    // check adding header won't overflow
    size_t aligned = ALIGN(user_size);
    if (aligned > SIZE_MAX - CHUNK_HEADER_SIZE) {
        errno = ENOMEM;
        return -1;
    }

    // make chunk be at least MIN_FREED_CHUNK_SIZE to fit the freed chunk
    // metadata once freed
    size_t chunk = aligned + CHUNK_HEADER_SIZE;
    if (chunk < MIN_FREED_CHUNK_SIZE)
        chunk = MIN_FREED_CHUNK_SIZE;

    *chunk_size = chunk;
    return 0;
}

void *_handle_large_alloc(size_t chunk_size) {

}

void *_handle_alloc(size_t chunk_size) {

}

void *malloc(size_t size) {
    void *chunk;
    const struct zone_info_s info = get_zone_infos(size);

    size_t chunk_size;
    if (_compute_chunk_size(size, &chunk_size) == -1)
        return NULL;

    // lock the zone mutex inside malloc
    // and don't touch the mutex anywhere else
    pthread_mutex_lock(info.lock);

    if (info.type == LARGE) {
        chunk = _handle_large_alloc(chunk_size);
    } else {
        chunk = _handle_alloc(chunk_size);
    }

    pthread_mutex_unlock(info.lock);
    return chunk;
}
