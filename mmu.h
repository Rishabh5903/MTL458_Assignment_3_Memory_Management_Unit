#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define MMAP_THRESHOLD (128 * 1024)  // mmap for large allocations (> 128KB)
#define MIN_ALLOC_SIZE 16  // Minimum block size to reduce fragmentation
#define ALIGNMENT 16  // Ensure 16-byte alignment for all allocations
#define MAX_ITERATIONS 1000000  // Maximum iterations for find_best_fit_block

typedef struct block_meta {
    size_t size;
    struct block_meta* next;
    struct block_meta* prev;
    int free;
    int mmaped;
} block_meta;

block_meta* global_base = NULL;

static size_t align(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Find the best-fit free block
static block_meta* find_best_fit_block(size_t size) {
    block_meta* best_fit = NULL;
    block_meta* current = global_base;
    int iterations = 0;
    while (current && iterations < MAX_ITERATIONS) {
        if (current->free && current->size >= size) {
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
        iterations++;
    }
    return best_fit;
}

// Request space from OS using sbrk for small blocks
static block_meta* request_space(block_meta* last, size_t size) {
    block_meta* block = sbrk(0);
    void* request = sbrk(size + sizeof(block_meta));
    if (request == (void*) -1) {
        return NULL; // sbrk failed
    }

    block->size = size;
    block->next = NULL;
    block->prev = last;
    block->free = 0;
    block->mmaped = 0;

    if (last) {
        last->next = block;
    }
    return block;
}

// Request space from OS using mmap for large blocks
static block_meta* request_space_mmap(size_t size) {
    block_meta* block = mmap(0, size + sizeof(block_meta), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) {
        return NULL; // mmap failed
    }
    block->size = size;
    block->next = NULL;
    block->prev = NULL;
    block->free = 0;
    block->mmaped = 1;
    return block;
}

// Split the block if there's excess space after allocation
static void split_block(block_meta* block, size_t size) {
    if (block->size >= size + sizeof(block_meta) + MIN_ALLOC_SIZE) {
        block_meta* new_block = (block_meta*)((char*)block + sizeof(block_meta) + size);
        new_block->size = block->size - size - sizeof(block_meta);
        new_block->next = block->next;
        new_block->prev = block;
        new_block->free = 1;
        new_block->mmaped = block->mmaped;
        block->size = size;

        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
    }
}

void* my_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t aligned_size = align(size);
    if (aligned_size < MIN_ALLOC_SIZE) aligned_size = MIN_ALLOC_SIZE;

    // Check for integer overflow
    if (aligned_size + sizeof(block_meta) < aligned_size) {
        errno = ENOMEM;
        return NULL;
    }

    block_meta* block;

    if (aligned_size + sizeof(block_meta) >= MMAP_THRESHOLD) {
        block = request_space_mmap(aligned_size);
        if (!block) {
            errno = ENOMEM;
            return NULL;
        }
    } else {
        block = find_best_fit_block(aligned_size);
        if (!block) {
            block_meta* last = global_base;
            if (last) {
                while (last->next) {
                    last = last->next;
                }
            }
            block = request_space(last, aligned_size);
            if (!block) {
                errno = ENOMEM;
                return NULL;
            }
        } else {
            block->free = 0;
            split_block(block, aligned_size);
        }
    }

    if (!global_base) {
        global_base = block;
    }

    return (void*)(block + 1);
}

// Coalesce adjacent free blocks
static void coalesce(block_meta* block) {
    // Coalesce with the next block if it's free and not mmaped
    if (block->next && block->next->free && !block->mmaped && !block->next->mmaped) {
        block->size += block->next->size + sizeof(block_meta);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    // Coalesce with the previous block if it's free and not mmaped
    if (block->prev && block->prev->free && !block->mmaped && !block->prev->mmaped) {
        block->prev->size += block->size + sizeof(block_meta);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

void my_free(void* ptr) {
    if (!ptr) {
        return;
    }

    block_meta* block_ptr = (block_meta*)ptr - 1;

    if (block_ptr->mmaped) {
        munmap(block_ptr, block_ptr->size + sizeof(block_meta));
    } else {
        block_ptr->free = 1;
        coalesce(block_ptr);
    }
}

void* my_calloc(size_t nmemb, size_t size) {
    size_t total_size;
    if (__builtin_mul_overflow(nmemb, size, &total_size)) {
        errno = ENOMEM;
        return NULL;
    }
    void* ptr = my_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}