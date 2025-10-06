#include <string.h>
#include "libft_malloc.h"

void	*realloc(void *ptr, size_t size) {

    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    chunk_header_t *meta = ptr - CHUNK_HEADER_SIZE;
    size_t old_size = meta->size - CHUNK_HEADER_SIZE;
    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size);
        free(ptr);
    }
    return new_ptr;
}
