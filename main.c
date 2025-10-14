#include <stdlib.h>

int main() {
    int *a = malloc(sizeof(int));
    int *b = malloc(sizeof(int));
    int *c = malloc(sizeof(int));
    *c = 5;



    free(b);

    free(a);
    free(c);
    return 0;
}
