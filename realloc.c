#include <string.h>
#include "libft_malloc.h"
#include <unistd.h>

void	*realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&mctx.g_lock);
    zone_metadata_t *zone;
    chunk_header_t *chunk;
    if (!search_pointer_in_heap(ptr, &zone, &chunk)) {
        return NULL;
    }
    pthread_mutex_unlock(&mctx.g_lock);

    size_t old_user_size = GET_RAW_SIZE(chunk) - mctx.HEADER_SIZE;

    size_t new_chunk_size;
    if (compute_chunk_size(size, &new_chunk_size) == -1) {
        return NULL;
    }

    if (new_chunk_size == GET_RAW_SIZE(chunk)) {
        return ptr; // Same chunk size, no need to reallocate
    }

    void *new_chunk = malloc(size);
    if (!new_chunk)
        return NULL;

    // Copy data: use MINIMUM of old and new sizes
    size_t copy_size = (old_user_size < size) ? old_user_size : size;
    ft_memmove(new_chunk, ptr, copy_size);

    free(ptr);
    return new_chunk;
}
