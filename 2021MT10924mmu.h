#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1)) // Aligns the size to the nearest multiple of ALIGNMENT
#define BLOCK_SIZE sizeof(block_t) // Defines the size of the block metadata structure

// Metadata structure for each memory block
typedef struct block {
    size_t size;           // Size of the block
    int free;              // Free flag: 1 if the block is free, 0 if in use
    struct block *next;    // Pointer to the next block in the list
    struct block *prev;    // Pointer to the previous block in the list
} block_t;

static block_t *free_list = NULL; // Head of the free list

// Function to allocate memory from the system using mmap
static void *allocate_from_system(size_t size) {
    size_t alloc_size = ALIGN(size + BLOCK_SIZE); // Align the requested size plus block metadata

    // Use mmap to request memory from the operating system
    void *block = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) {
        return NULL; // Return NULL if mmap fails
    }
    return block;
}

// Find a free block in the free list that is large enough for the requested size
static block_t *find_free_block(block_t **last, size_t size) {
    block_t *current = free_list;
    // Traverse the free list to find a suitable block
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current; // Return the found block or NULL if no suitable block was found
}

// Split the block if it's large enough to hold the requested size plus another block
static void split_block(block_t *block, size_t size) {
    if (block->size >= size + BLOCK_SIZE + ALIGNMENT) {
        block_t *new_block = (block_t *)((char *)block + size + BLOCK_SIZE);
        new_block->size = block->size - size - BLOCK_SIZE; // Update size of the new block
        new_block->free = 1; // Mark the new block as free
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) block->next->prev = new_block;
        block->size = size; // Update size of the current block
        block->next = new_block; // Link the new block to the list
    }
}

// Coalesce adjacent free blocks to avoid fragmentation
static void coalesce() {
    block_t *current = free_list;
    // Traverse the free list and merge adjacent free blocks
    while (current && current->next) {
        if (current->free && current->next->free &&
            (char *)current + current->size + BLOCK_SIZE == (char *)current->next) {
            current->size += BLOCK_SIZE + current->next->size; // Merge blocks
            current->next = current->next->next; // Update next pointer
            if (current->next) {
                current->next->prev = current; // Update previous pointer of the next block
            }
        } else {
            current = current->next; // Move to the next block
        }
    }
}

// Custom malloc function to allocate memory
void *my_malloc(size_t size) {
    if (size == 0) return NULL; // Return NULL for zero-size allocation

    size_t aligned_size = ALIGN(size); // Align the requested size
    block_t *block, *last = NULL;

    // Try to find a free block in the free list
    if ((block = find_free_block(&last, aligned_size))) {
        block->free = 0; // Mark the block as in use
        split_block(block, aligned_size); // Split the block if necessary
    } else {
        size_t alloc_size = aligned_size + BLOCK_SIZE;

        // Allocate a new block from the system
        block = allocate_from_system(alloc_size);
        if (!block) return NULL; // Return NULL if allocation fails

        block->size = alloc_size - BLOCK_SIZE; // Set the size of the allocated block
        block->free = 0; // Mark the block as in use
        block->next = NULL;
        block->prev = last;

        // Add the new block to the free list
        if (!free_list) {
            free_list = block;
        } else {
            last->next = block;
        }

        split_block(block, aligned_size); // Split the block if necessary
    }

    return (void *)(block + 1); // Return a pointer to the memory region after the block metadata
}

// Custom calloc function to allocate and zero-initialize memory
void *my_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size; // Calculate total memory size
    void *ptr = my_malloc(total_size); // Allocate the memory
    if (ptr) {
        memset(ptr, 0, total_size); // Zero-initialize the memory
    }
    return ptr; // Return the allocated and initialized memory
}

// Custom free function to free allocated memory
void my_free(void *ptr) {
    if (!ptr) return; // Do nothing if the pointer is NULL

    block_t *block = (block_t *)ptr - 1; // Get the block metadata
    block->free = 1; // Mark the block as free

    coalesce(); // Coalesce adjacent free blocks

    // If the block is the only one in the free list and it's free, unmap it
    if (block->prev == NULL && block->next == NULL) {
        munmap(block, block->size + BLOCK_SIZE); // Unmap the memory
        free_list = NULL; // Reset the free list
    }
}

// Custom realloc function to resize allocated memory
void *my_realloc(void *ptr, size_t size) {
    if (!ptr) return my_malloc(size); // Allocate new memory if the pointer is NULL
    if (size == 0) {
        my_free(ptr); // Free the memory if size is 0
        return NULL;
    }

    block_t *block = (block_t *)ptr - 1; // Get the block metadata
    if (block->size >= size) {
        split_block(block, size); // Split the block if the new size is smaller
        return ptr; // Return the original pointer
    }

    // Allocate new memory if the block is too small
    void *new_ptr = my_malloc(size);
    if (!new_ptr) return NULL; // Return NULL if allocation fails

    memcpy(new_ptr, ptr, block->size); // Copy data to the new memory
    my_free(ptr); // Free the old block

    return new_ptr; // Return the new pointer
}
