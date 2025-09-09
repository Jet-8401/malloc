#include "libft_malloc.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdalign.h>

#define STRESS_ALLOCATIONS 10000
#define LARGE_ALLOC_SIZE (SMALL_ZONE_TRESHOLD + 1)

// Helper to print test results
void print_test_result(const char* test_name, int success) {
    printf("%-40s: %s\n", test_name, success ? "PASSED" : "FAILED");
}

// Test 1: Basic allocation and deallocation
void test_basic_functionality() {
    int success = 1;
    char *str = (char *)malloc(10);
    if (!str) {
        success = 0;
    } else {
        strcpy(str, "hello");
        if (strcmp(str, "hello") != 0) {
            success = 0;
        }
        free(str);
    }
    print_test_result("Basic Functionality", success);
}

// Test 2: Test different zones
void test_zone_allocations() {
    int success = 1;

    // TINY
    char *tiny = (char *)malloc(TINY_ZONE_TRESHOLD - 1);
    if (!tiny) success = 0;

    // SMALL
    char *small = (char *)malloc(SMALL_ZONE_TRESHOLD - 1);
    if (!small) success = 0;

    // LARGE
    char *large = (char *)malloc(LARGE_ALLOC_SIZE);
    if (!large) success = 0;

    if (tiny) {
        strcpy(tiny, "tiny");
        if (strcmp(tiny, "tiny") != 0) success = 0;
    }
    if (small) {
        strcpy(small, "small");
        if (strcmp(small, "small") != 0) success = 0;
    }
    if (large) {
        strcpy(large, "large");
        if (strcmp(large, "large") != 0) success = 0;
    }

    if (tiny) free(tiny);
    if (small) free(small);
    if (large) free(large);

    print_test_result("Zone Allocations (TINY, SMALL, LARGE)", success);
}

// Test 3: Memory Reuse
void test_memory_reuse() {
    int success = 1;
    void *ptr1 = malloc(10);
    if (!ptr1) {
        success = 0;
    }
    free(ptr1);
    void *ptr2 = malloc(10); // Should ideally reuse ptr1
    if (!ptr2) {
        success = 0;
    }
    free(ptr2);

    // This test is conceptual. The main point is that the sequence should not crash.
    print_test_result("Memory Reuse", success);
}

// Test 4: Data Integrity
void test_data_integrity() {
    int success = 1;
    int *data = (int *)malloc(100 * sizeof(int));
    if (!data) {
        success = 0;
    } else {
        for (int i = 0; i < 100; i++) {
            data[i] = i;
        }
        for (int i = 0; i < 100; i++) {
            if (data[i] != i) {
                success = 0;
                break;
            }
        }
        free(data);
    }
    print_test_result("Data Integrity", success);
}

// Test 5: Alignment
void test_alignment() {
    int success = 1;
    void *ptr = malloc(1);
    if (!ptr) {
        success = 0;
    } else {
        // Check if the pointer is aligned to _Alignof(max_align_t)
        if ((uintptr_t)ptr % _Alignof(max_align_t) != 0) {
            success = 0;
            printf("Alignment failed: ptr=%p, align=%zu\n", ptr, _Alignof(max_align_t));
        }
        free(ptr);
    }
    print_test_result("Pointer Alignment", success);
}

// Test 6: Heavy Stress Test
void test_stress() {
    int success = 1;
    void **allocations = (void **)malloc(STRESS_ALLOCATIONS * sizeof(void *));
    size_t *sizes = (size_t *)malloc(STRESS_ALLOCATIONS * sizeof(size_t));

    if (!allocations || !sizes) {
        print_test_result("Heavy Stress Test (Setup Failed)", 0);
        if (allocations) free(allocations);
        if (sizes) free(sizes);
        return;
    }

    srand(time(NULL));

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
        memset(allocations[i], i % 256, sizes[i]);
    }

    // Verify data and free in a different order
    for (int i = STRESS_ALLOCATIONS - 1; i >= 0; i--) {
        if (allocations[i]) {
            unsigned char *data = (unsigned char *)allocations[i];
            for (size_t j = 0; j < sizes[i]; j++) {
                if (data[j] != (unsigned char)(i % 256)) {
                    success = 0;
                    printf("Stress test data corruption at allocation %d\n", i);
                    goto free_and_cleanup;
                }
            }
        }
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

int main() {
    printf("--- Malloc Test Suite ---\n");

    test_basic_functionality();
    test_zone_allocations();
    test_memory_reuse();
    test_data_integrity();
    test_alignment();
    test_stress();

    printf("--- Test Suite Finished ---\n");

    // Final check: one last allocation to see if the heap is still usable
    int *final_check = malloc(sizeof(int));
    if (!final_check) {
        print_test_result("Final Malloc Usability Check", 0);
    } else {
        *final_check = 123;
        if (*final_check == 123) {
            print_test_result("Final Malloc Usability Check", 1);
        } else {
            print_test_result("Final Malloc Usability Check", 0);
        }
        free(final_check);
    }

    return 0;
}
