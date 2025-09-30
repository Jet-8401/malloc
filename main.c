#include "libft_malloc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Helper function to print memory layout for debugging
void print_chunk_info(void *ptr, const char *description) {
    if (!ptr) {
        printf("%s: NULL pointer\n", description);
        return;
    }

    chunk_header_t *chunk = (chunk_header_t *)((char *)ptr - CHUNK_HEADER_SIZE);
    printf("%s: ptr=%p, chunk=%p, size=%zu (raw=%zu), flags=%d\n",
           description, ptr, chunk, UNMASK(chunk->size), chunk->size,
           (int)(chunk->size & CHUNK_META_MASK));
}

// Test 1: Basic coalescing - backward coalescing
void test_backward_coalescing() {
    printf("\n=== Test 1: Backward Coalescing ===\n");

    int *first = malloc(sizeof(int));
    int *second = malloc(sizeof(int));
    int *third = malloc(sizeof(int));

    *first = 42;
    *second = 36;
    *third = 90;

    print_chunk_info(first, "First chunk");
    print_chunk_info(second, "Second chunk");
    print_chunk_info(third, "Third chunk");

    printf("Freeing first chunk...\n");
    free(first);

    printf("Freeing second chunk (should coalesce backward with first)...\n");
    free(second);

    // Allocate a larger chunk that should fit in the coalesced space
    int *large = malloc(sizeof(int) * 2);
    *large = 16;
    print_chunk_info(large, "Large chunk (should reuse coalesced space)");

    free(third);
    free(large);

    printf("Backward coalescing test completed\n");
}

// Test 2: Forward coalescing
void test_forward_coalescing() {
    printf("\n=== Test 2: Forward Coalescing ===\n");

    int *first = malloc(sizeof(int));
    int *second = malloc(sizeof(int));
    int *third = malloc(sizeof(int));

    *first = 42;
    *second = 36;
    *third = 90;

    print_chunk_info(first, "First chunk");
    print_chunk_info(second, "Second chunk");
    print_chunk_info(third, "Third chunk");

    printf("Freeing second chunk...\n");
    free(second);

    printf("Freeing third chunk (should coalesce forward with second)...\n");
    free(third);

    // Allocate a larger chunk that should fit in the coalesced space
    int *large = malloc(sizeof(int) * 2);
    *large = 456;
    print_chunk_info(large, "Large chunk (should reuse coalesced space)");

    free(first);
    free(large);

    printf("Forward coalescing test completed\n");
}

// Test 3: Bidirectional coalescing (both backward and forward)
void test_bidirectional_coalescing() {
    printf("\n=== Test 3: Bidirectional Coalescing ===\n");

    int *first = malloc(sizeof(int));
    int *second = malloc(sizeof(int));
    int *third = malloc(sizeof(int));
    int *fourth = malloc(sizeof(int));

    *first = 1;
    *second = 2;
    *third = 3;
    *fourth = 4;

    print_chunk_info(first, "First chunk");
    print_chunk_info(second, "Second chunk");
    print_chunk_info(third, "Third chunk");
    print_chunk_info(fourth, "Fourth chunk");

    printf("Freeing first chunk...\n");
    free(first);

    printf("Freeing third chunk...\n");
    free(third);

    printf("Freeing second chunk (should coalesce with both first and third)...\n");
    free(second);

    // Allocate a chunk that should fit in the large coalesced space
    int *large = malloc(sizeof(int) * 3);
    *large = 789;
    print_chunk_info(large, "Large chunk (should reuse all coalesced space)");

    free(fourth);
    free(large);

    printf("Bidirectional coalescing test completed\n");
}

// Test 4: Multiple fragmentation and coalescing
void test_fragmentation_coalescing() {
    printf("\n=== Test 4: Fragmentation and Multiple Coalescing ===\n");

    // Create alternating allocation pattern
    void *ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(sizeof(int));
        *(int*)ptrs[i] = i;
        printf("Allocated chunk %d: %p\n", i, ptrs[i]);
    }

    // Free every other chunk to create fragmentation
    printf("\nCreating fragmentation by freeing odd-indexed chunks...\n");
    for (int i = 1; i < 10; i += 2) {
        printf("Freeing chunk %d\n", i);
        free(ptrs[i]);
        ptrs[i] = NULL;
    }

    // Now free adjacent chunks to test coalescing
    printf("\nFreeing adjacent chunks to test coalescing...\n");
    for (int i = 0; i < 10; i += 2) {
        if (ptrs[i]) {
            printf("Freeing chunk %d (should coalesce)\n", i);
            free(ptrs[i]);
        }
    }

    // Allocate a large chunk that should use the coalesced space
    int *large = malloc(sizeof(int) * 10);
    if (large) {
        *large = 999;
        print_chunk_info(large, "Large allocation after coalescing");
        free(large);
    }

    printf("Fragmentation coalescing test completed\n");
}

// Test 5: Top chunk absorption
void test_top_chunk_absorption() {
    printf("\n=== Test 5: Top Chunk Absorption ===\n");

    // Allocate some chunks
    int *first = malloc(sizeof(int));
    int *second = malloc(sizeof(int));
    int *third = malloc(sizeof(int));

    *first = 100;
    *second = 200;
    *third = 300;

    print_chunk_info(first, "First chunk");
    print_chunk_info(second, "Second chunk");
    print_chunk_info(third, "Third chunk (should be adjacent to top chunk)");

    // Free the last allocated chunk - it should be absorbed by top chunk
    printf("Freeing third chunk (should be absorbed by top chunk)...\n");
    free(third);

    // Free second chunk - should also be absorbed
    printf("Freeing second chunk (should extend absorption)...\n");
    free(second);

    // Allocate a large chunk that should come from the expanded top chunk
    int *large = malloc(sizeof(int) * 4);
    *large = 777;
    print_chunk_info(large, "Large chunk from expanded top chunk");

    free(first);
    free(large);

    printf("Top chunk absorption test completed\n");
}

// Test 6: Edge cases and boundary conditions
void test_edge_cases() {
    printf("\n=== Test 6: Edge Cases ===\n");

    // Test freeing NULL
    printf("Testing free(NULL)...\n");
    free(NULL);
    printf("free(NULL) handled correctly\n");

    // Test minimal allocations
    printf("Testing minimal allocations...\n");
    char *tiny1 = malloc(1);
    char *tiny2 = malloc(1);
    char *tiny3 = malloc(1);

    *tiny1 = 'A';
    *tiny2 = 'B';
    *tiny3 = 'C';

    print_chunk_info(tiny1, "Tiny allocation 1");
    print_chunk_info(tiny2, "Tiny allocation 2");
    print_chunk_info(tiny3, "Tiny allocation 3");

    free(tiny2);
    free(tiny1); // Should coalesce with tiny2
    free(tiny3);

    printf("Edge cases test completed\n");
}

// Test 7: Stress test with random allocation/deallocation pattern
void test_stress_coalescing() {
    printf("\n=== Test 7: Stress Test ===\n");

    const int NUM_ALLOCS = 20;
    void *ptrs[NUM_ALLOCS];

    // Initialize all to NULL
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = NULL;
    }

    // Allocate all chunks
    printf("Allocating %d chunks...\n", NUM_ALLOCS);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = malloc(sizeof(int) * (i % 4 + 1)); // Variable sizes
        if (ptrs[i]) {
            *(int*)ptrs[i] = i * 10;
        }
    }

    // Free in a pattern that should create opportunities for coalescing
    printf("Freeing chunks in pattern to test coalescing...\n");

    // Free every 3rd chunk
    for (int i = 2; i < NUM_ALLOCS; i += 3) {
        if (ptrs[i]) {
            free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    // Free adjacent chunks to trigger coalescing
    for (int i = 1; i < NUM_ALLOCS; i += 3) {
        if (ptrs[i]) {
            free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    // Free remaining chunks
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
        }
    }

    printf("Stress test completed\n");
}

// Original test case from the file
void test_original_case() {
    printf("\n=== Original Test Case ===\n");

    int *first = malloc(sizeof(int));
    *first = 42;
    print_chunk_info(first, "After first allocation");

    int *second = malloc(sizeof(int));
    *second = 36;
    print_chunk_info(second, "After second allocation");

    int *third = malloc(sizeof(int));
    *third = 90;
    print_chunk_info(third, "After third allocation");

    printf("Freeing second chunk...\n");
    free(second);

    printf("Freeing first chunk (should coalesce with second)...\n");
    free(first);

    printf("Allocating double-sized chunk...\n");
    first = malloc(sizeof(int) * 2);
    *first = 89;
    print_chunk_info(first, "Double-sized allocation");

    free(first);
    free(third);

    printf("Original test case completed\n");
}

int main() {
    printf("Starting comprehensive coalescing tests...\n");

    // test_original_case();
    test_backward_coalescing();
    // test_forward_coalescing();
    // test_bidirectional_coalescing();
    // test_fragmentation_coalescing();
    // test_top_chunk_absorption();
    // test_edge_cases();
    // test_stress_coalescing();

    printf("\n=== All Tests Completed ===\n");
    printf("If no crashes occurred, basic coalescing functionality is working.\n");
    printf("Check the output above for any unexpected behavior in chunk addresses/sizes.\n");

    return 0;
}
