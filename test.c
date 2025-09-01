#include <stdalign.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#define TINY_TRESHOLD 128
#define SMALL_TRESHOLD 1024

// size_t size = (requested + alignment - 1) & ~(alignment - 1);

int main() {
    const unsigned long page_size = sysconf(_SC_PAGESIZE);
    printf("pagesize: %ld\n", page_size);
    const unsigned long alignment = alignof(max_align_t);
    printf("memory alignement: %ld\n", alignment);
    return 0;
}
