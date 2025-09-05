#include "libft_malloc.h"
#include <unistd.h>

int main() {
    int *value = malloc(sizeof(int));
    if (!value)
        return 1;
    *value = 52;

    free(value);

    return 0;
}
