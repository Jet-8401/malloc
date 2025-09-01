#include "libft_malloc.h"

#include <stdio.h>
#include <unistd.h>

int main() {
    int *value = malloc(sizeof(int));
    if (!value)
        return 1;
    *value = 52;

    int *second_value = malloc(sizeof(int));
    if (!second_value)
        return 1;
    *second_value = 103;

    for (int i = 0; i < 2048; i++) {
        int *temp_value = malloc(sizeof(int));
        printf("%p\n", (void*) temp_value);
        if (!temp_value)
            return 1;
        *temp_value = i;
    }

    return 0;
}
