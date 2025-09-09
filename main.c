// #include "libft_malloc.h"
#include <stddef.h>
#include <stdlib.h>

int main() {
    const size_t alloc_size = 4096;

    const size_t iterations = 80;
    void* addresses[iterations];

    for (int i = 0; i < iterations; i++) {
        addresses[i] = malloc(alloc_size);
    }

    int *value = malloc(sizeof(int));
    *value = 50;

    for (int i = 0; i < iterations; i++) {
        free(addresses[i]);
    }

    free(value);

    return 0;
}
