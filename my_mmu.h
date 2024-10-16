#ifndef _2020MT60986MMU_H
#define _2020MT60986MMU_H

#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define BLOCK_SIZE sizeof(block_t)
#define MIN_ALLOC_SIZE (128 * 1024)  // 128KB minimum allocation

typedef struct block {
    size_t size;
    int free;
    struct block *next;
    struct block *prev;  
} block_t;

static block_t *free_list = NULL;

// Debug counters
static struct {
    size_t malloc_calls;
    size_t calloc_calls;
    size_t free_calls;
    size_t realloc_calls;
    size_t mmap_calls;
    size_t munmap_calls;
    size_t split_blocks;
    size_t coalesce_operations;
} debug_counters = {0};

static void *allocate_from_system(size_t size) {
    size_t alloc_size = (size > MIN_ALLOC_SIZE) ? size : MIN_ALLOC_SIZE;
    alloc_size = ALIGN(alloc_size);
    
    debug_counters.mmap_calls++;
    void *block = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) {
        return NULL;
    }
    return block;
}

static block_t *find_free_block(block_t **last, size_t size) {
    block_t *current = free_list;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

static void split_block(block_t *block, size_t size) {
    block_t *new_block;
    if (block->size >= size + BLOCK_SIZE + ALIGNMENT) {
        debug_counters.split_blocks++;
        new_block = (block_t *)((char *)block + size + BLOCK_SIZE);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->free = 1;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

static void coalesce() {
    block_t *current = free_list;
    while (current && current->next) {
        if (current->free && current->next->free &&
            (char *)current + current->size + BLOCK_SIZE == (char *)current->next) {
            debug_counters.coalesce_operations++;
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void *my_malloc(size_t size) {
    debug_counters.malloc_calls++;
    if (size == 0) return NULL;

    size_t aligned_size = ALIGN(size);
    block_t *block, *last = NULL;

    if ((block = find_free_block(&last, aligned_size))) {
        block->free = 0;
        split_block(block, aligned_size);
    } else {
        size_t alloc_size = (aligned_size + BLOCK_SIZE > MIN_ALLOC_SIZE) ? 
                            (aligned_size + BLOCK_SIZE) : MIN_ALLOC_SIZE;
        
        block = allocate_from_system(alloc_size);
        if (!block) return NULL;

        block->size = alloc_size - BLOCK_SIZE;
        block->free = 0;
        block->next = NULL;
        block->prev = last;

        if (!free_list) {
            free_list = block;
        } else {
            last->next = block;
        }

        split_block(block, aligned_size);
    }

    return (void *)(block + 1);
}

void *my_calloc(size_t nmemb, size_t size) {
    debug_counters.calloc_calls++;
    size_t total_size = nmemb * size;
    void *ptr = my_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void my_free(void *ptr) {
    debug_counters.free_calls++;
    if (!ptr) return;

    block_t *block = (block_t *)ptr - 1;
    block->free = 1;

    coalesce();

    // Check if this is the only block and it's free
    if (block->prev == NULL && block->next == NULL) {
        debug_counters.munmap_calls++;
        munmap(block, block->size + BLOCK_SIZE);
        free_list = NULL;
    }
}

void *my_realloc(void *ptr, size_t size) {
    debug_counters.realloc_calls++;
    if (!ptr) return my_malloc(size);
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_t *block = (block_t *)ptr - 1;
    if (block->size >= size) {
        split_block(block, size);
        return ptr;
    }

    void *new_ptr = my_malloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);

    return new_ptr;
}

// Function to print debug counters
void print_debug_counters() {
    printf("Debug Counters:\n");
    printf("  malloc calls: %zu\n", debug_counters.malloc_calls);
    printf("  calloc calls: %zu\n", debug_counters.calloc_calls);
    printf("  free calls: %zu\n", debug_counters.free_calls);
    printf("  realloc calls: %zu\n", debug_counters.realloc_calls);
    printf("  mmap calls: %zu\n", debug_counters.mmap_calls);
    printf("  munmap calls: %zu\n", debug_counters.munmap_calls);
    printf("  split blocks: %zu\n", debug_counters.split_blocks);
    printf("  coalesce operations: %zu\n", debug_counters.coalesce_operations);
}

#endif // _2020MT60986MMU_H