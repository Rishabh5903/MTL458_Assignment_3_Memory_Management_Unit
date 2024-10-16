#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "my_mmu.h"

// Timer function to calculate elapsed time
double calculate_time_taken(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

// Check memory alignment (typically 8 or 16 bytes)
void test_alignment() {
    printf("Testing memory alignment...\n");
    clock_t start = clock();

    size_t alignment = sizeof(void*);
    void* ptr = my_malloc(16);
    uintptr_t address = (uintptr_t)ptr;
    
    if (address % alignment == 0) {
        printf("Memory is correctly aligned to %zu bytes.\n", alignment);
    } else {
        printf("Memory alignment error!\n");
    }
    my_free(ptr);

    clock_t end = clock();
    printf("Time taken for test_alignment: %.6f seconds\n", calculate_time_taken(start, end));
}

// Function to test various allocation sizes and memory reuse
void test_fragmentation() {
    printf("Testing fragmentation and memory reuse...\n");
    clock_t start = clock();

    void* blocks[10];

    // Allocate blocks of different sizes
    blocks[0] = my_malloc(32);
    blocks[1] = my_malloc(64);
    blocks[2] = my_malloc(128);
    blocks[3] = my_malloc(256);
    blocks[4] = my_malloc(512);

    // Free some blocks to create fragmentation
    my_free(blocks[1]);
    my_free(blocks[3]);

    // Reuse freed memory
    void* new_block = my_malloc(64);
    if (new_block == blocks[1]) {
        printf("Successfully reused freed block of size 64 bytes.\n");
    } else {
        printf("Failed to reuse memory efficiently!\n");
    }

    // Clean up remaining allocations
    my_free(blocks[0]);
    my_free(blocks[2]);
    my_free(blocks[4]);
    my_free(new_block);

    clock_t end = clock();
    printf("Time taken for test_fragmentation: %.6f seconds\n", calculate_time_taken(start, end));
}

// Function to test large memory allocations (near system limits)
void test_large_allocations() {
    printf("Testing large allocations...\n");
    clock_t start = clock();

    size_t large_size = 1024 * 1024 * 50; // 50 MB

    void* large_block = my_malloc(large_size);
    if (large_block) {
        printf("Successfully allocated large block of 50 MB.\n");
        memset(large_block, 0, large_size);  // Ensure we can use the block
        my_free(large_block);
    } else {
        printf("Failed to allocate large block!\n");
    }

    clock_t end = clock();
    printf("Time taken for test_large_allocations: %.6f seconds\n", calculate_time_taken(start, end));
}

// Function to test random allocation and freeing pattern
void test_random_allocations() {
    printf("Testing random allocation and freeing pattern...\n");
    clock_t start = clock();

    srand(time(NULL));
    void* blocks[100];
    size_t sizes[100];

    // Randomly allocate blocks of memory
    for (int i = 0; i < 100; i++) {
        sizes[i] = (rand() % 512) + 1;  // Random sizes between 1 and 512 bytes
        blocks[i] = my_malloc(sizes[i]);
        if (blocks[i] == NULL) {
            printf("Allocation failed at iteration %d\n", i);
        }
    }

    // Randomly free half of the blocks
    for (int i = 0; i < 100; i += 2) {
        my_free(blocks[i]);
    }

    // Allocate again and see if fragmentation causes any issues
    for (int i = 0; i < 50; i++) {
        blocks[i] = my_malloc(sizes[i] * 2);  // Allocate larger blocks
        if (!blocks[i]) {
            printf("Failed to allocate larger block in iteration %d\n", i);
        }
    }

    // Clean up remaining blocks
    for (int i = 0; i < 100; i++) {
        if (blocks[i]) {
            my_free(blocks[i]);
        }
    }

    clock_t end = clock();
    printf("Time taken for test_random_allocations: %.6f seconds\n", calculate_time_taken(start, end));
}

// Function to test boundary conditions (allocating page size, 0 bytes, etc.)
void test_boundary_conditions() {
    printf("Testing boundary conditions...\n");
    clock_t start = clock();

    // Test malloc(0)
    void* ptr_zero = my_malloc(0);
    if (ptr_zero != NULL) {
        printf("my_malloc(0) should return NULL!\n");
    } else {
        printf("my_malloc(0) correctly returned NULL.\n");
    }

    // Test free(NULL)
    my_free(NULL);
    printf("my_free(NULL) passed (no crash).\n");

    // Test allocating very large block (close to system limits)
    size_t large_size = 1024 * 1024 * 512;  // 512 MB
    void* large_ptr = my_malloc(large_size);
    if (large_ptr) {
        printf("Allocated 512 MB successfully.\n");
        my_free(large_ptr);
    } else {
        printf("Failed to allocate 512 MB block (system limit reached).\n");
    }

    clock_t end = clock();
    printf("Time taken for test_boundary_conditions: %.6f seconds\n", calculate_time_taken(start, end));
}

// Stress test with increased iterations
void stress_test(int iterations) {
    printf("Starting stress test with %d iterations...\n", iterations);
    clock_t start = clock();

    void* blocks[1000];

    // Allocate and free many blocks of random sizes
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < 1000; j++) {
            size_t size = (rand() % 512) + 1;  // Random sizes between 1 and 512 bytes
            blocks[j] = my_malloc(size);
            if (!blocks[j]) {
                printf("my_malloc failed at iteration %d, block %d\n", i, j);
            }
        }

        // Free all allocated blocks
        for (int j = 0; j < 1000; j++) {
            my_free(blocks[j]);
        }
    }

    clock_t end = clock();
    printf("Stress test completed in %.6f seconds.\n", calculate_time_taken(start, end));
}

int main() {
    // Run your tests here
    test_alignment();
    test_fragmentation();
    test_large_allocations();
    test_random_allocations();
    test_boundary_conditions();
    stress_test(500);

    // Print debug information
    // print_debug_counters();

    return 0;
}