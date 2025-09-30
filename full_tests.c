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
    // Your implementation enforces minimum 8-byte alignment
    const size_t expected_align = (_Alignof(max_align_t) > 8) ? _Alignof(max_align_t) : 8;

    void *ptr = malloc(1);
    if (!ptr) {
        success = 0;
    } else {
        // Check if the pointer is aligned to the expected alignment
        if ((uintptr_t)ptr % expected_align != 0) {
            success = 0;
            printf("Alignment failed: ptr=%p, expected_align=%zu, actual_offset=%zu\n",
                   ptr, expected_align, (uintptr_t)ptr % expected_align);
        }
        free(ptr);
    }
    print_test_result("Basic Pointer Alignment", success);
}

// Test 5a: Comprehensive Alignment Tests
void test_comprehensive_alignment() {
    int success = 1;
    // Your implementation enforces minimum 8-byte alignment
    const size_t expected_align = (_Alignof(max_align_t) > 8) ? _Alignof(max_align_t) : 8;

    // Test various allocation sizes
    size_t test_sizes[] = {1, 2, 3, 4, 5, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256};
    size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (size_t i = 0; i < num_sizes; i++) {
        void *ptr = malloc(test_sizes[i]);
        if (!ptr) {
            success = 0;
            printf("Alignment test failed: malloc(%zu) returned NULL\n", test_sizes[i]);
            break;
        }

        if ((uintptr_t)ptr % expected_align != 0) {
            success = 0;
            printf("Alignment failed for size %zu: ptr=%p, expected alignment=%zu, actual offset=%zu\n",
                   test_sizes[i], ptr, expected_align, (uintptr_t)ptr % expected_align);
        }

        free(ptr);
    }

    print_test_result("Comprehensive Alignment Tests", success);
}

// Test 5b: Multiple Allocation Alignment
void test_multiple_allocation_alignment() {
    int success = 1;
    // Your implementation enforces minimum 8-byte alignment
    const size_t expected_align = (_Alignof(max_align_t) > 8) ? _Alignof(max_align_t) : 8;
    const int num_allocs = 20;
    void *ptrs[20];

    // Allocate multiple blocks and check each one
    for (int i = 0; i < num_allocs; i++) {
        size_t size = (i % 10) + 1; // Sizes 1-10
        ptrs[i] = malloc(size);

        if (!ptrs[i]) {
            success = 0;
            printf("Multiple allocation test failed: malloc(%zu) returned NULL at iteration %d\n", size, i);
            // Free previous allocations
            for (int j = 0; j < i; j++) {
                free(ptrs[j]);
            }
            break;
        }

        if ((uintptr_t)ptrs[i] % expected_align != 0) {
            success = 0;
            printf("Multiple allocation alignment failed at iteration %d: ptr=%p, size=%zu, offset=%zu\n",
                   i, ptrs[i], size, (uintptr_t)ptrs[i] % expected_align);
        }
    }

    // Free all allocations if we got this far
    if (success) {
        for (int i = 0; i < num_allocs; i++) {
            free(ptrs[i]);
        }
    }

    print_test_result("Multiple Allocation Alignment", success);
}

// Test 5c: Alignment for Different Zone Types
void test_zone_alignment() {
    int success = 1;
    // Your implementation enforces minimum 8-byte alignment
    const size_t expected_align = (_Alignof(max_align_t) > 8) ? _Alignof(max_align_t) : 8;

    // Test TINY zone
    void *tiny_ptr = malloc(TINY_ZONE_TRESHOLD - 1);
    if (!tiny_ptr) {
        success = 0;
        printf("Zone alignment test failed: TINY allocation returned NULL\n");
    } else if ((uintptr_t)tiny_ptr % expected_align != 0) {
        success = 0;
        printf("TINY zone alignment failed: ptr=%p, offset=%zu\n",
               tiny_ptr, (uintptr_t)tiny_ptr % expected_align);
    }

    // Test SMALL zone
    void *small_ptr = malloc(SMALL_ZONE_TRESHOLD - 1);
    if (!small_ptr) {
        success = 0;
        printf("Zone alignment test failed: SMALL allocation returned NULL\n");
    } else if ((uintptr_t)small_ptr % expected_align != 0) {
        success = 0;
        printf("SMALL zone alignment failed: ptr=%p, offset=%zu\n",
               small_ptr, (uintptr_t)small_ptr % expected_align);
    }

    // Test LARGE zone
    void *large_ptr = malloc(LARGE_ALLOC_SIZE);
    if (!large_ptr) {
        success = 0;
        printf("Zone alignment test failed: LARGE allocation returned NULL\n");
    } else if ((uintptr_t)large_ptr % expected_align != 0) {
        success = 0;
        printf("LARGE zone alignment failed: ptr=%p, offset=%zu\n",
               large_ptr, (uintptr_t)large_ptr % expected_align);
    }

    // Clean up
    if (tiny_ptr) free(tiny_ptr);
    if (small_ptr) free(small_ptr);
    if (large_ptr) free(large_ptr);

    print_test_result("Zone-Specific Alignment", success);
}

// Test 5d: Alignment After Fragmentation
void test_alignment_after_fragmentation() {
    int success = 1;
    // Your implementation enforces minimum 8-byte alignment
    const size_t expected_align = (_Alignof(max_align_t) > 8) ? _Alignof(max_align_t) : 8;
    const int num_ptrs = 10;
    void *ptrs[10];

    // Create fragmentation by allocating and freeing every other block
    for (int i = 0; i < num_ptrs; i++) {
        ptrs[i] = malloc(16); // Small consistent size
        if (!ptrs[i]) {
            success = 0;
            printf("Fragmentation test setup failed at allocation %d\n", i);
            break;
        }
    }

    // Free every other block to create fragmentation
    for (int i = 1; i < num_ptrs; i += 2) {
        free(ptrs[i]);
        ptrs[i] = NULL;
    }

    // Now allocate new blocks that should fill the gaps
    for (int i = 1; i < num_ptrs; i += 2) {
        ptrs[i] = malloc(16);
        if (!ptrs[i]) {
            success = 0;
            printf("Fragmentation test reallocation failed at index %d\n", i);
            break;
        }

        if ((uintptr_t)ptrs[i] % expected_align != 0) {
            success = 0;
            printf("Alignment failed after fragmentation at index %d: ptr=%p, offset=%zu\n",
                   i, ptrs[i], (uintptr_t)ptrs[i] % expected_align);
        }
    }

    // Clean up all remaining allocations
    for (int i = 0; i < num_ptrs; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
        }
    }

    print_test_result("Alignment After Fragmentation", success);
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
    test_data_integrity();
    test_alignment();
    test_comprehensive_alignment();
    test_multiple_allocation_alignment();
    test_zone_alignment();
    test_alignment_after_fragmentation();
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
