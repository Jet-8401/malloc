#include <stdlib.h>

// for (int i = 0; i < 100; i++) {
//     int *a = malloc(sizeof(int));
// }

int *array[100];

int main() {
    for (int i = 0; i < 100; i++) {
        array[i] = malloc(sizeof(int));
    }

    for (int i = 100; i >= 0; i--) {
        free(array[i]);
    }

    return 0;
}
