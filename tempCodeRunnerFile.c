#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "mmu.h"

#define NUM_ALLOCS 5000 // Number of allocations to perform
#define MAX_ALLOC_SIZE 10000 // Maximum size for allocation

// Stress test 1: Random allocations and deallocations with various sizes
void test_stress_memory_allocations() {
    printf("Stress test 1: Random memory allocations and deallocations...\n");
    
    void* allocations[NUM_ALLOCS] = { NULL }; // Keep track of allocated pointers

    // Seed random number generator
    srand((unsigned int)time(NULL));

    // Allocate memory of random sizes
    for (int i = 0; i < NUM_ALLOCS; i++) {
        size_t size = rand() % MAX_ALLOC_SIZE + 1; // Random size between 1 and MAX_ALLOC_SIZE
        allocations[i] = my_malloc(size);
        assert(allocations[i] != NULL); // Ensure allocation succeeded
        // if (allocations[i] == NULL) {
        //     printf("Allocation failed at index %d, skipping.\n", i);
        //     continue;
        // }

        // Optionally write to the allocated memory to ensure it's usable
        memset(allocations[i], i % 256, size);
    }

    // Randomly free half of the allocations
    for (int i = 0; i < NUM_ALLOCS / 2; i++) {
        int index = rand() % NUM_ALLOCS;
        if (allocations[index] != NULL) {
            my_free(allocations[index]);
            allocations[index] = NULL;
        }
    }

    // Allocate again to force memory reuse
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (allocations[i] == NULL) {
            size_t size = rand() % MAX_ALLOC_SIZE + 1;
            allocations[i] = my_malloc(size);
            assert(allocations[i] != NULL);
            // if (allocations[i] == NULL) {
            //     printf("Allocation failed at index %d, skipping.\n", i);
            //     continue;
            // }

            
            // Optionally write to the allocated memory again
            memset(allocations[i], i % 256, size);
        }
    }

    // Free all remaining allocations
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (allocations[i] != NULL) {
            my_free(allocations[i]);
            allocations[i] = NULL;
        }
    }

    printf("Stress test 1 passed: Random allocations and deallocations\n");
}

// Stress test 2: Allocate large blocks, free them, and test merging
void test_large_block_allocation_and_merging() {
    printf("Stress test 2: Large block allocations and merging...\n");
    
    const int num_blocks = 100;
    void* blocks[num_blocks];

    // Allocate large blocks of memory
    for (int i = 0; i < num_blocks; i++) {
        blocks[i] = my_malloc(MAX_ALLOC_SIZE);
        assert(blocks[i] != NULL);

        // Write to the block to ensure it's usable
        memset(blocks[i], i, MAX_ALLOC_SIZE);
    }

    // Free every other block
    for (int i = 0; i < num_blocks; i += 2) {
        my_free(blocks[i]);
    }

    // Allocate large blocks again, this should trigger reuse and merging of freed blocks
    for (int i = 0; i < num_blocks; i++) {
        if (i % 2 == 0) {
            blocks[i] = my_malloc(MAX_ALLOC_SIZE / 2);
            assert(blocks[i] != NULL);

            // Optionally write to the new block
            memset(blocks[i], i, MAX_ALLOC_SIZE / 2);
        }
    }

    // Free all blocks
    for (int i = 0; i < num_blocks; i++) {
        my_free(blocks[i]);
    }

    printf("Stress test 2 passed: Large block allocations and merging\n");
}

// Stress test 3: Heavy interleaved calloc and malloc usage
void test_interleaved_malloc_calloc() {
    printf("Stress test 3: Interleaved malloc and calloc...\n");

    const int iterations = 5000;
    void* pointers[iterations];
    size_t sizes[iterations];

    // Interleave malloc and calloc with varying sizes
    for (int i = 0; i < iterations; i++) {
        if (i % 2 == 0) {
            sizes[i] = rand() % MAX_ALLOC_SIZE + 1;
            pointers[i] = my_malloc(sizes[i]);
        } else {
            sizes[i] = rand() % (MAX_ALLOC_SIZE / 2) + 1;
            pointers[i] = my_calloc(sizes[i], sizeof(char));
        }

        assert(pointers[i] != NULL);

        // Optionally write to allocated memory
        memset(pointers[i], i % 256, sizes[i]);
    }

    // Free all allocations
    for (int i = 0; i < iterations; i++) {
        my_free(pointers[i]);
    }

    printf("Stress test 3 passed: Interleaved malloc and calloc\n");
}

// Fragmentation Test: Allocating and freeing in a way that creates fragmentation
void test_memory_fragmentation() {
    printf("Testing memory fragmentation...\n");

    const int num_allocations = 10000;
    void* allocations[num_allocations];

    // Allocate memory in a way that leads to fragmentation
    for (int i = 0; i < num_allocations; i++) {
        size_t size = (i % 10 + 1) * 1000; // Vary sizes to cause fragmentation
        allocations[i] = my_malloc(size);
        assert(allocations[i] != NULL);
    }

    // Free every odd-indexed allocation to create gaps
    for (int i = 1; i < num_allocations; i += 2) {
        my_free(allocations[i]);
        allocations[i] = NULL;
    }

    // Check for fragmentation by allocating again
    size_t total_size = 0;
    for (int i = 0; i < num_allocations; i++) {
        if (allocations[i] != NULL) {
            total_size += (i % 10 + 1) * 1000;
        }
    }

    // Try to allocate more memory and check if we can still fit
    void* extra_allocation = my_malloc(total_size);
    if (extra_allocation == NULL) {
        printf("Fragmentation detected: Could not allocate extra memory\n");
    } else {
        printf("No fragmentation detected: Extra allocation succeeded\n");
        my_free(extra_allocation);
    }

    // Free all remaining allocations
    for (int i = 0; i < num_allocations; i++) {
        if (allocations[i] != NULL) {
            my_free(allocations[i]);
        }
    }

    printf("Fragmentation test completed.\n");
}


int main() {
    // Run all the stress tests
    test_stress_memory_allocations();
    test_large_block_allocation_and_merging();
    test_interleaved_malloc_calloc();
    test_memory_fragmentation();

    printf("All stress tests passed successfully.\n");
    return 0;
}