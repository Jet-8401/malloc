#include "libft_malloc.h"

// Note: When freeing a point we need to check for the chunks surrounding it
// inside memory to coalesce it if needed.
void free(void *ptr) {
    if (ptr == NULL)
        return;

    return;
}
