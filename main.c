#include "libft_malloc.h"

/* To compile use LD_PRELOAD=./libft_malloc.so
 * This tells the dynamic linker to load your library first,
 * so libft_malloc `malloc` will be used instead of the system one.
 */

int main() {
    int *value = ft_malloc(sizeof(int));
    if (!value)
        return 1;
    *value = 52;

    int *second_value = ft_malloc(sizeof(int));
    if (!second_value)
        return 1;
    *second_value = 103;

    for (int i = 0; i < 1023; i++) {
        int *temp_value = ft_malloc(sizeof(int));
        if (!temp_value)
            return 1;
        *temp_value = i;
    }

    return 0;
}
