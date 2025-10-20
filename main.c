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
    int *big_alloc = malloc(n);
    memset(big_alloc, 'a', n);

    return 0;
}
