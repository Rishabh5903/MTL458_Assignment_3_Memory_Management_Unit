#ifndef MMU_H
#define MMU_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define BLOCK_SIZE 4096
#define MIN_ALLOC_SIZE 32

typedef struct block_meta {
    size_t size;
    struct block_meta* next;
    int free;
} block_meta;

void* global_base = NULL;

// Function to find a free block of memory
block_meta* find_free_block(block_meta** last, size_t size) {
    block_meta* current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

// Function to request more memory from the system
block_meta* request_space(block_meta* last, size_t size) {
    block_meta* block;
    block = mmap(NULL, size + sizeof(block_meta), 
                 PROT_READ | PROT_WRITE, 
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (block == MAP_FAILED) {
        return NULL;
    }
    
    if (last) {
        last->next = block;
    }
    
    block->size = size;
    block->next = NULL;
    block->free = 0;
    return block;
}

// Function to allocate memory using mmap
void* my_malloc(size_t size) {
    block_meta* block;
    
    if (size <= 0) {
        return NULL;
    }
    
    if (size < MIN_ALLOC_SIZE) {
        size = MIN_ALLOC_SIZE;
    }
    
    if (!global_base) {
        block = request_space(NULL, size);
        if (!block) {
            return NULL;
        }
        global_base = block;
    } else {
        block_meta* last = global_base;
        block = find_free_block(&last, size);
        if (!block) {
            block = request_space(last, size);
            if (!block) {
                return NULL;
            }
        } else {
            block->free = 0;
        }
    }
    
    return (block + 1);
}

// Function to allocate and initialize memory to zero using mmap
void* my_calloc(size_t nelem, size_t size) {
    size_t total_size = nelem * size;
    void* ptr = my_malloc(total_size);
    
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

// Function to release memory allocated using my_malloc and my_calloc
void my_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    block_meta* block_ptr = ((block_meta*)ptr) - 1;
    block_ptr->free = 1;
    
    // Attempt to merge adjacent free blocks
    block_meta* current = global_base;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(block_meta);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

#endif // MMU_H