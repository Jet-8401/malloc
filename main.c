#include "libft_malloc.h"
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define STRESS_ALLOCATIONS 10000

void print_test_result(const char* test_name, int success) {
    printf("%-40s: %s\n", test_name, success ? "PASSED" : "FAILED");
}

void stress_test() {
    int success = 1;
    void **allocations = (void **)malloc(STRESS_ALLOCATIONS * sizeof(void *));
    size_t *sizes = (size_t *)malloc(STRESS_ALLOCATIONS * sizeof(size_t));

    if (!allocations || !sizes) {
        print_test_result("Heavy Stress Test (Setup Failed)", 0);
        if (allocations) free(allocations);
        if (sizes) free(sizes);
        return;
    }

    // Allocate many chunks of random sizes
    for (int i = 0; i < STRESS_ALLOCATIONS; i++) {
        sizes[i] = rand() % (SMALL_ZONE_TRESHOLD * 2);
        if (sizes[i] == 0) sizes[i] = 1; // Ensure we allocate at least 1 byte
        allocations[i] = malloc(sizes[i]);

        if (!allocations[i]) {
            success = 0;
            char buffer[1024];
            sprintf(buffer, "Stress test malloc failed at iteration %d for size %zu\n", i, sizes[i]);
            perror(buffer);
            // Free up to the point of failure
            for (int j = 0; j < i; j++) {
                free(allocations[j]);
            }
            goto cleanup;
        }
        // Write some data to verify integrity later
        // memset(allocations[i], i % 256, sizes[i]);
    }

free_and_cleanup:
    for (int i = 0; i < STRESS_ALLOCATIONS; i++) {
        free(allocations[i]);
    }

cleanup:
    free(allocations);
    free(sizes);
    print_test_result("Heavy Stress Test", success);
}

void tiny_and_small_zones() {
    void* addresses[STRESS_ALLOCATIONS];


    for (int i = 0; i < STRESS_ALLOCATIONS; i++) {
        size_t size = rand() % SMALL_ZONE_TRESHOLD;
        if (size == 0)
            size = 1;

        addresses[i] = malloc(size);

        if (addresses[i] == NULL) {
            exit(1);
        }
    }
}

int main() {
    srand(time(NULL));
    stress_test();
    // tiny_and_small_zones();
}
