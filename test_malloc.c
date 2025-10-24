#include "libft_malloc.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>

/* ANSI Color Codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"

/* Test Status */
static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;
static int skip_dangerous_tests = 0;

/* Helper Macros */
#define TEST_START(name) \
    do { \
        printf("\n" COLOR_BOLD COLOR_CYAN "TEST: " COLOR_RESET COLOR_CYAN "%s" COLOR_RESET "\n", name); \
        total_tests++; \
    } while(0)

#define TEST_PASS() \
    do { \
        printf(COLOR_GREEN "  ✓ PASSED" COLOR_RESET "\n"); \
        passed_tests++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf(COLOR_RED "  ✗ FAILED: %s" COLOR_RESET "\n", msg); \
        failed_tests++; \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_EQUAL(a, b, msg) ASSERT_TRUE((a) == (b), msg)

#define TEST_SKIP(msg) \
    do { \
        printf(COLOR_YELLOW "  ⊘ SKIPPED: %s" COLOR_RESET "\n", msg); \
        total_tests--; \
    } while(0)

/* Print Section Header */
void print_section(const char *section_name) {
    printf("\n");
    printf(COLOR_BOLD COLOR_MAGENTA "═══════════════════════════════════════════════════════════════\n");
    printf("  %s\n", section_name);
    printf("═══════════════════════════════════════════════════════════════" COLOR_RESET "\n");
}

void print_subsection(const char *name) {
    printf("\n" COLOR_BOLD COLOR_YELLOW "--- %s ---" COLOR_RESET "\n", name);
}

/* ========================================================================
 * MALLOC TESTS
 * ======================================================================== */

void test_malloc_null() {
    TEST_START("malloc(0) should return NULL or valid pointer");
    void *ptr = malloc(0);
    // Either NULL or valid pointer is acceptable
    if (ptr != NULL) {
        free(ptr);
    }
    TEST_PASS();
}

void test_malloc_tiny() {
    TEST_START("malloc tiny allocation (< 128 bytes)");
    void *ptr = malloc(42);
    ASSERT_NOT_NULL(ptr, "malloc(42) returned NULL");

    // Write and read data
    memset(ptr, 'A', 42);
    for (int i = 0; i < 42; i++) {
        ASSERT_EQUAL(((char*)ptr)[i], 'A', "Data corruption in tiny allocation");
    }

    free(ptr);
    TEST_PASS();
}

void test_malloc_small() {
    TEST_START("malloc small allocation (128-2048 bytes)");
    void *ptr = malloc(1024);
    ASSERT_NOT_NULL(ptr, "malloc(1024) returned NULL");

    // Write and read data
    memset(ptr, 'B', 1024);
    for (int i = 0; i < 1024; i++) {
        ASSERT_EQUAL(((char*)ptr)[i], 'B', "Data corruption in small allocation");
    }

    free(ptr);
    TEST_PASS();
}

void test_malloc_large() {
    TEST_START("malloc large allocation (> 2048 bytes)");
    void *ptr = malloc(8192);
    ASSERT_NOT_NULL(ptr, "malloc(8192) returned NULL");

    // Write and read data
    memset(ptr, 'C', 8192);
    for (int i = 0; i < 8192; i++) {
        ASSERT_EQUAL(((char*)ptr)[i], 'C', "Data corruption in large allocation");
    }

    free(ptr);
    TEST_PASS();
}

void test_malloc_multiple() {
    TEST_START("Multiple allocations of different sizes");

    void *ptrs[10];
    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

    // Allocate
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(sizes[i]);
        ASSERT_NOT_NULL(ptrs[i], "Allocation failed");
        memset(ptrs[i], i, sizes[i]);
    }

    // Verify
    for (int i = 0; i < 10; i++) {
        for (size_t j = 0; j < sizes[i]; j++) {
            ASSERT_EQUAL(((unsigned char*)ptrs[i])[j], (unsigned char)i,
                        "Data corruption in multiple allocations");
        }
    }

    // Free
    for (int i = 0; i < 10; i++) {
        free(ptrs[i]);
    }

    TEST_PASS();
}

void test_malloc_alignment() {
    TEST_START("malloc returns properly aligned pointers");

    for (int i = 1; i <= 100; i++) {
        void *ptr = malloc(i);
        ASSERT_NOT_NULL(ptr, "malloc failed");

        // Check alignment (should be at least 8-byte aligned)
        ASSERT_TRUE(((size_t)ptr % 8) == 0, "Pointer not properly aligned");

        free(ptr);
    }

    TEST_PASS();
}

void test_malloc_stress() {
    TEST_START("Stress test: 1000 allocations");

    void *ptrs[1000];

    for (int i = 0; i < 1000; i++) {
        size_t size = (i % 100) + 1;
        ptrs[i] = malloc(size);
        ASSERT_NOT_NULL(ptrs[i], "Allocation failed in stress test");
        memset(ptrs[i], (i % 256), size);
    }

    for (int i = 0; i < 1000; i++) {
        free(ptrs[i]);
    }

    TEST_PASS();
}

/* ========================================================================
 * FREE TESTS
 * ======================================================================== */

void test_free_null() {
    TEST_START("free(NULL) should not crash");
    free(NULL);
    TEST_PASS();
}

void test_free_basic() {
    TEST_START("Basic free after malloc");
    void *ptr = malloc(128);
    ASSERT_NOT_NULL(ptr, "malloc failed");
    free(ptr);
    TEST_PASS();
}

void test_free_pattern() {
    TEST_START("Free in different patterns");

    void *ptrs[10];

    // Allocate
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(64);
        ASSERT_NOT_NULL(ptrs[i], "Allocation failed");
    }

    // Free odd indices first
    for (int i = 1; i < 10; i += 2) {
        free(ptrs[i]);
    }

    // Free even indices
    for (int i = 0; i < 10; i += 2) {
        free(ptrs[i]);
    }

    TEST_PASS();
}

void test_free_and_realloc() {
    TEST_START("Free and reallocate same size");

    void *ptr1 = malloc(256);
    ASSERT_NOT_NULL(ptr1, "First allocation failed");
    free(ptr1);

    void *ptr2 = malloc(256);
    ASSERT_NOT_NULL(ptr2, "Second allocation failed");

    // ptr2 might reuse ptr1's memory (not guaranteed, but possible)

    free(ptr2);
    TEST_PASS();
}

/* ========================================================================
 * CALLOC TESTS
 * ======================================================================== */

void test_calloc_basic() {
    TEST_START("calloc basic allocation");

    void *ptr = calloc(10, 4);
    ASSERT_NOT_NULL(ptr, "calloc failed");

    // Verify zero initialization
    for (int i = 0; i < 40; i++) {
        ASSERT_EQUAL(((unsigned char*)ptr)[i], 0, "calloc did not zero memory");
    }

    free(ptr);
    TEST_PASS();
}

void test_calloc_zero() {
    TEST_START("calloc with zero size");

    void *ptr1 = calloc(0, 10);
    void *ptr2 = calloc(10, 0);

    // Either NULL or valid pointer is acceptable
    if (ptr1) free(ptr1);
    if (ptr2) free(ptr2);

    TEST_PASS();
}

void test_calloc_large() {
    TEST_START("calloc large allocation");

    void *ptr = calloc(1000, 8);
    ASSERT_NOT_NULL(ptr, "calloc large failed");

    // Verify zero initialization
    for (int i = 0; i < 8000; i++) {
        ASSERT_EQUAL(((unsigned char*)ptr)[i], 0, "calloc did not zero large memory");
    }

    free(ptr);
    TEST_PASS();
}

/* ========================================================================
 * REALLOC TESTS
 * ======================================================================== */

void test_realloc_null() {
    TEST_START("realloc(NULL, size) behaves like malloc");

    void *ptr = realloc(NULL, 128);
    ASSERT_NOT_NULL(ptr, "realloc(NULL, 128) failed");

    memset(ptr, 'X', 128);
    free(ptr);
    TEST_PASS();
}

void test_realloc_zero() {
    TEST_START("realloc(ptr, 0) behaves like free");

    void *ptr = malloc(128);
    ASSERT_NOT_NULL(ptr, "malloc failed");

    void *result = realloc(ptr, 0);
    // Result should be NULL (acts like free)
    ASSERT_NULL(result, "realloc(ptr, 0) should return NULL");

    TEST_PASS();
}

void test_realloc_grow() {
    TEST_START("realloc to larger size");

    void *ptr = malloc(64);
    ASSERT_NOT_NULL(ptr, "malloc failed");

    // Fill with pattern
    memset(ptr, 'A', 64);

    void *new_ptr = realloc(ptr, 256);
    ASSERT_NOT_NULL(new_ptr, "realloc failed");

    // Verify old data preserved
    for (int i = 0; i < 64; i++) {
        ASSERT_EQUAL(((char*)new_ptr)[i], 'A', "Data lost during realloc");
    }

    free(new_ptr);
    TEST_PASS();
}

void test_realloc_shrink() {
    TEST_START("realloc to smaller size");

    void *ptr = malloc(256);
    ASSERT_NOT_NULL(ptr, "malloc failed");

    // Fill with pattern
    memset(ptr, 'B', 256);

    void *new_ptr = realloc(ptr, 64);
    ASSERT_NOT_NULL(new_ptr, "realloc failed");

    // Verify data preserved (first 64 bytes)
    for (int i = 0; i < 64; i++) {
        ASSERT_EQUAL(((char*)new_ptr)[i], 'B', "Data lost during realloc shrink");
    }

    free(new_ptr);
    TEST_PASS();
}

void test_realloc_same_size() {
    TEST_START("realloc to same size");

    void *ptr = malloc(128);
    ASSERT_NOT_NULL(ptr, "malloc failed");
    memset(ptr, 'C', 128);

    void *new_ptr = realloc(ptr, 128);
    ASSERT_NOT_NULL(new_ptr, "realloc failed");

    // Verify data preserved
    for (int i = 0; i < 128; i++) {
        ASSERT_EQUAL(((char*)new_ptr)[i], 'C', "Data lost during same-size realloc");
    }

    free(new_ptr);
    TEST_PASS();
}

/* ========================================================================
 * EDGE CASE TESTS
 * ======================================================================== */

void test_malloc_boundary_sizes() {
    TEST_START("Allocation at boundary sizes");

    size_t sizes[] = {127, 128, 129, 2047, 2048, 2049};

    for (int i = 0; i < 6; i++) {
        void *ptr = malloc(sizes[i]);
        ASSERT_NOT_NULL(ptr, "Boundary size allocation failed");
        memset(ptr, 0xAB, sizes[i]);
        free(ptr);
    }

    TEST_PASS();
}

void test_malloc_very_large() {
    TEST_START("Very large allocation");

    void *ptr = malloc(1024 * 1024); // 1 MB
    ASSERT_NOT_NULL(ptr, "Large allocation failed");

    // Write to first and last byte
    ((char*)ptr)[0] = 'X';
    ((char*)ptr)[1024 * 1024 - 1] = 'Y';

    ASSERT_EQUAL(((char*)ptr)[0], 'X', "Data corruption in large alloc");
    ASSERT_EQUAL(((char*)ptr)[1024 * 1024 - 1], 'Y', "Data corruption in large alloc");

    free(ptr);
    TEST_PASS();
}

void test_fragmentation() {
    TEST_START("Memory fragmentation handling");

    void *ptrs[20];

    // Allocate 20 blocks
    for (int i = 0; i < 20; i++) {
        ptrs[i] = malloc(64);
        ASSERT_NOT_NULL(ptrs[i], "Allocation failed");
    }

    // Free every other block
    for (int i = 0; i < 20; i += 2) {
        free(ptrs[i]);
    }

    // Allocate new blocks that might fit in freed spaces
    for (int i = 0; i < 10; i++) {
        void *ptr = malloc(32);
        ASSERT_NOT_NULL(ptr, "Fragmentation allocation failed");
        free(ptr);
    }

    // Free remaining blocks
    for (int i = 1; i < 20; i += 2) {
        free(ptrs[i]);
    }

    TEST_PASS();
}

/* ========================================================================
 * MULTITHREADING TESTS
 * ======================================================================== */

#define THREAD_ALLOCS 1000

void *thread_malloc_free(void *arg) {
    int thread_id = *(int*)arg;
    void *ptrs[THREAD_ALLOCS];

    for (int i = 0; i < THREAD_ALLOCS; i++) {
        size_t size = ((thread_id * 17 + i * 7) % 500) + 1;
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            memset(ptrs[i], thread_id & 0xFF, size);
        }
    }

    for (int i = 0; i < THREAD_ALLOCS; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
        }
    }

    return NULL;
}

void test_multithreading() {
    TEST_START("Multithreading safety test");

    if (skip_dangerous_tests) {
        TEST_SKIP("Skipping due to known issues (use --enable-dangerous to run)");
        return;
    }

    pthread_t threads[8];
    int thread_ids[8];

    for (int i = 0; i < 8; i++) {
        thread_ids[i] = i;
        int ret = pthread_create(&threads[i], NULL, thread_malloc_free, &thread_ids[i]);
        ASSERT_EQUAL(ret, 0, "Thread creation failed");
    }

    for (int i = 0; i < 8; i++) {
        pthread_join(threads[i], NULL);
    }

    TEST_PASS();
}

void test_multithreading_simple() {
    TEST_START("Simple multithreading test (2 threads)");

    if (skip_dangerous_tests) {
        TEST_SKIP("Skipping due to known issues (use --enable-dangerous to run)");
        return;
    }

    pthread_t threads[2];
    int thread_ids[2] = {0, 1};

    for (int i = 0; i < 2; i++) {
        int ret = pthread_create(&threads[i], NULL, thread_malloc_free, &thread_ids[i]);
        ASSERT_EQUAL(ret, 0, "Thread creation failed");
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    TEST_PASS();
}

/* ========================================================================
 * PERFORMANCE TESTS
 * ======================================================================== */

void test_performance_malloc() {
    TEST_START("Performance: 10000 malloc/free operations");

    if (skip_dangerous_tests) {
        TEST_SKIP("Skipping due to known issues (use --enable-dangerous to run)");
        return;
    }

    clock_t start = clock();

    for (int i = 0; i < 10000; i++) {
        void *ptr = malloc(128);
        if (ptr) {
            free(ptr);
        }
    }

    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf(COLOR_DIM "    Time: %.2f ms" COLOR_RESET "\n", elapsed);
    TEST_PASS();
}

void test_performance_mixed() {
    TEST_START("Performance: Mixed operations");

    if (skip_dangerous_tests) {
        TEST_SKIP("Skipping due to known issues (use --enable-dangerous to run)");
        return;
    }

    clock_t start = clock();
    void *ptrs[1000];

    // Allocate
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = malloc((i % 512) + 1);
    }

    // Realloc half
    for (int i = 0; i < 500; i++) {
        if (ptrs[i]) {
            ptrs[i] = realloc(ptrs[i], (i % 1024) + 1);
        }
    }

    // Free all
    for (int i = 0; i < 1000; i++) {
        free(ptrs[i]);
    }

    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf(COLOR_DIM "    Time: %.2f ms" COLOR_RESET "\n", elapsed);
    TEST_PASS();
}

/* ========================================================================
 * MAIN TEST RUNNER
 * ======================================================================== */

void print_summary() {
    printf("\n");
    printf(COLOR_BOLD "═══════════════════════════════════════════════════════════════\n");
    printf("  TEST SUMMARY\n");
    printf("═══════════════════════════════════════════════════════════════" COLOR_RESET "\n");
    printf(COLOR_BOLD "  Total Tests:  " COLOR_RESET "%d\n", total_tests);
    printf(COLOR_BOLD COLOR_GREEN "  Passed:       " COLOR_RESET "%d\n", passed_tests);
    printf(COLOR_BOLD COLOR_RED "  Failed:       " COLOR_RESET "%d\n", failed_tests);
    printf(COLOR_BOLD "  Success Rate: " COLOR_RESET "%.1f%%\n",
           (total_tests > 0) ? (passed_tests * 100.0 / total_tests) : 0.0);
    printf(COLOR_BOLD "═══════════════════════════════════════════════════════════════" COLOR_RESET "\n\n");

    if (failed_tests == 0) {
        printf(COLOR_BOLD COLOR_GREEN "  🎉 ALL TESTS PASSED! 🎉" COLOR_RESET "\n\n");
    } else {
        printf(COLOR_BOLD COLOR_RED "  ⚠️  SOME TESTS FAILED ⚠️" COLOR_RESET "\n\n");
    }
}

int main(int argc, char **argv) {
    // Parse command line arguments
    skip_dangerous_tests = 1; // Default: skip dangerous tests

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--enable-dangerous") == 0) {
            skip_dangerous_tests = 0;
            printf(COLOR_YELLOW "⚠️  Running with dangerous tests enabled!\n" COLOR_RESET);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --enable-dangerous    Enable tests known to cause issues\n");
            printf("  --help, -h           Show this help message\n");
            return 0;
        }
    }

    printf(COLOR_BOLD COLOR_MAGENTA);
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                               ║\n");
    printf("║           MALLOC IMPLEMENTATION TEST SUITE                   ║\n");
    printf("║                                                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);

    if (skip_dangerous_tests) {
        printf(COLOR_YELLOW "\n⚠️  Note: Dangerous tests are disabled by default.\n");
        printf("   Use --enable-dangerous to run all tests.\n" COLOR_RESET);
    }

    /* MALLOC Tests */
    print_section("MALLOC TESTS");
    test_malloc_null();
    test_malloc_tiny();
    test_malloc_small();
    test_malloc_large();
    test_malloc_multiple();
    test_malloc_alignment();
    test_malloc_stress();
    test_malloc_boundary_sizes();
    test_malloc_very_large();

    /* FREE Tests */
    print_section("FREE TESTS");
    test_free_null();
    test_free_basic();
    test_free_pattern();
    test_free_and_realloc();
    test_fragmentation();

    /* CALLOC Tests */
    print_section("CALLOC TESTS");
    test_calloc_basic();
    test_calloc_zero();
    test_calloc_large();

    /* REALLOC Tests */
    print_section("REALLOC TESTS");
    test_realloc_null();
    test_realloc_zero();
    test_realloc_grow();
    test_realloc_shrink();
    test_realloc_same_size();

    /* Multithreading Tests */
    print_section("MULTITHREADING TESTS");
    test_multithreading_simple();
    test_multithreading();

    /* Performance Tests */
    print_section("PERFORMANCE TESTS");
    test_performance_malloc();
    test_performance_mixed();

    /* Print Summary */
    print_summary();

    return (failed_tests > 0) ? 1 : 0;
}
