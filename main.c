#include <stdlib.h>
#include <string.h>

int main() {
    int *a = malloc(sizeof(int));
    *a = 10;
    int *b = malloc(sizeof(int));
    *b = 5;
    int *c = malloc(sizeof(int));
    *c = 84;

    const size_t n = 500000;
    void *big_alloc = malloc(n);
    memset(big_alloc, 'a', n);

    const size_t m = 800000;
    void *other_big_alloc = malloc(m);
    memset(other_big_alloc, 'b', m);

    free(other_big_alloc);
    free(big_alloc);

    return 0;
}
