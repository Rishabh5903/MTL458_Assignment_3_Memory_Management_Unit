#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include<stddef.h>
#include "my_mmu.h"

// Global counters for tracking function calls
static int malloc_count = 0;
static int calloc_count = 0;
static int free_count = 0;

// Wrapper for my_malloc to count calls
void* my_malloc_wrapper(size_t size) {
    malloc_count++;
    return my_malloc(size); // Call the original my_malloc
}

// Wrapper for my_calloc to count calls
void* my_calloc_wrapper(size_t nmemb, size_t size) {
    calloc_count++;
    return my_calloc(nmemb, size); // Call the original my_calloc
}

// Wrapper for my_free to count calls
void my_free_wrapper(void* ptr) {
    free_count++;
    my_free(ptr); // Call the original my_free
}

// Function to reset the counters
void reset_counters() {
    malloc_count = 0;
    calloc_count = 0;
    free_count = 0;
}

// Function to test memory allocation and deallocation
void test_memory_management() {
    reset_counters();

    printf("Running memory management tests...\n");

    // Test my_malloc and my_free
    void* ptr1 = my_malloc_wrapper(32);
    assert(ptr1 != NULL);
    printf("\n%p",(void*)ptr1);
    my_free_wrapper(ptr1);

    // Test my_calloc and my_free
    void* ptr2 = my_calloc_wrapper(4, 8); // Allocate space for 4 elements of 8 bytes each
    assert(ptr2 != NULL);
    printf("\n%p",(void*)ptr2);
    my_free_wrapper(ptr2);
    printf("\n%p",(void*)ptr2);

    void* ptrx = (void *)malloc(64);
    printf("\n%p",(void*)ptrx);
    free(ptrx);
    printf("\n%p",(void*)ptrx);

    // Test memory reuse
    void* ptr3 = my_malloc_wrapper(64);
    printf("\n%p",(void*)ptr3);
    my_free_wrapper(ptr3);
    printf("\n%p",(void*)ptr3);

    void* ptr4 = my_malloc_wrapper(64); // This should reuse the freed block
    assert(ptr4 != NULL);
    printf("\n%p",(void*)ptr4);
    my_free_wrapper(ptr4);

    void* ptry = (void *)malloc(64);
    printf("\n%p",(void*)ptry);
    free(ptry);
    printf("\n%p",(void*)ptry);

    // Test multiple allocations
    void* blocks[10];
    for (int i = 0; i < 10; i++) {
        blocks[i] = my_malloc_wrapper(128);
        assert(blocks[i] != NULL);
        printf("\n%p",(void*)blocks[i]);
    }

    // Free every other block to test fragmentation and coalescing
    for (int i = 0; i < 10; i += 2) {
        my_free_wrapper(blocks[i]);
    }

    // Allocate again to check if coalescing works
    void* ptr5 = my_malloc_wrapper(64);
    assert(ptr5 != NULL);
    printf("\n%p",(void*)ptr5);

    // Free all remaining blocks
    for (int i = 1; i < 10; i += 2) {
        my_free_wrapper(blocks[i]);
    }
    
    // Free the last allocated block
    my_free_wrapper(ptr5);

    // Print function call counts
    printf("Function call counts:\n");
    printf("my_malloc called: %d times\n", malloc_count);
    printf("my_calloc called: %d times\n", calloc_count);
    printf("my_free called: %d times\n", free_count);
    // printf("next coalesce called: %d times\n", nextcol);
    // printf("prev coalesce called: %d times\n", prevcol);
    // printf("entering coalesce : %d times\n", entercol);




    // Validate coalescing by checking if free blocks can be reused
    void* ptr6 = my_malloc_wrapper(128);
    assert(ptr6 != NULL);
    printf("\n%p",(void*)ptr6);
    my_free_wrapper(ptr6);
}

int main() {
    test_memory_management();
    printf("All tests passed successfully!\n");
    return 0;
}