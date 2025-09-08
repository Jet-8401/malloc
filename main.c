#include "libft_malloc.h"
#include <unistd.h>

int main() {
    int *value = malloc(sizeof(int));
    if (!value)
        return 1;
    *value = 45;

    free(value);

    value = malloc(sizeof(int));
    *value = 102;
    free(value);

    value = malloc(sizeof(int));
    if (!value)
        return 1;

    free(value);

    return 0;
}
