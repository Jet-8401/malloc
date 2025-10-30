#include "libft_malloc.h"
#include <stdlib.h>

int main() {
    // {
    //     int *a = malloc(sizeof(int));
    //     *a = 10;
    //     int *b = malloc(sizeof(int));
    //     *b = 5;
    //     int *c = malloc(sizeof(int));
    //     *c = 84;

    //     const size_t n = 500000;
    //     void *big_alloc = malloc(n);
    //     memset(big_alloc, 'a', n);

    //     free(big_alloc);

    //     const size_t m = 800000;
    //     void *other_big_alloc = malloc(m);
    //     memset(other_big_alloc, 'b', m);

    // 	// added comment through vim
    //     free(other_big_alloc);
    // }
    // {
    //     int *a = malloc(sizeof(int)); // 0x100
    //     int *b = malloc(sizeof(int)); // 0x101

    //     free(b);    // free 0x101 (append front)
    //     free(a);    // free 0x100 (append front)

    //     b = malloc(sizeof(int));    // should have 0x100
    //     a = malloc(sizeof(int));    // should have 0x101

    //     free(a);
    //     free(b);
    // }
    // {
    //     // coalescing testing
    //     int *a = malloc(sizeof(int));

    //     show_alloc_mem();

    //     int *b = malloc(sizeof(int));
    //     int *c = malloc(sizeof(int));

    //     *a = 50;
    //     *b = 50933;
    //     *c = -1;

    //     show_alloc_mem();

    //     free(a);
    //     free(b);

    //     show_alloc_mem();

    //     int *d = malloc(sizeof(int));
    //     int *e = malloc(1024 * 1024);

    //     show_alloc_mem();

    //     free(c);
    //     free(d);
    //     free(e);

    //     show_alloc_mem();
    // }
    // {
    //     void *large_alloc = malloc(1024 * 1024);

    //     show_alloc_mem();

    //     free(large_alloc);
    // }
    {
        int *a = malloc(96);

        show_alloc_mem();

        int *b = malloc(sizeof(int));
        *b = 303949339;

        a = realloc(a, 32); // 64 bytes freed block

        show_alloc_mem();

        int *c = malloc(32);

        show_alloc_mem();

        free(a);
        free(b);
        free(c);
    }
    return 0;
}
