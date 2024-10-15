#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define MMAP_THRESHOLD (128 * 1024)  // Use mmap for allocations larger than 128KB

typedef struct block_meta {
    size_t size;
    struct block_meta* next;
    struct block_meta* prev;
    int free;
    int mmaped;
} block_meta;

block_meta* global_base = NULL;
block_meta* mmap_list = NULL;

static size_t align(size_t size) {
    return (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
}

static block_meta* find_free_block(block_meta** last, size_t size) {
    block_meta* current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

static block_meta* request_space(block_meta* last, size_t size) {
    block_meta* block;
    block = sbrk(0);
    void* request = sbrk(size + sizeof(block_meta));
    if (request == (void*) -1) {
        return NULL; // sbrk failed
    }

    if (last) {
        last->next = block;
    }
    block->size = size;
    block->next = NULL;
    block->prev = last;
    block->free = 0;
    block->mmaped = 0;
    return block;
}

static block_meta* request_space_mmap(size_t size) {
    block_meta* block;
    block = mmap(0, size + sizeof(block_meta), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) {
        return NULL;
    }
    block->size = size;
    block->next = mmap_list;
    block->prev = NULL;
    if (mmap_list) {
        mmap_list->prev = block;
    }
    block->free = 0;
    block->mmaped = 1;
    mmap_list = block;
    return block;
}

static block_meta* find_free_mmap_block(size_t size) {
    block_meta* current = mmap_list;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void split_block(block_meta* block, size_t size) {
    if (block->size - size >= sizeof(block_meta) + sizeof(size_t)) {
        block_meta* new_block = (block_meta*)((char*)block + sizeof(block_meta) + size);
        new_block->size = block->size - size - sizeof(block_meta);
        new_block->next = block->next;
        new_block->prev = block;
        new_block->free = 1;
        new_block->mmaped = block->mmaped;
        
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        block->size = size;
    }
}

static void coalesce_mmap(block_meta* block) {
    // Coalesce with next block if it's free
    if (block->next && block->next->free && block->next->mmaped) {
        block->size += block->next->size + sizeof(block_meta);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Coalesce with previous block if it's free
    if (block->prev && block->prev->free && block->prev->mmaped) {
        block->prev->size += block->size + sizeof(block_meta);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

void* my_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t aligned_size = align(size);
    block_meta* block;

    if (size + sizeof(block_meta) >= MMAP_THRESHOLD) {
        block = find_free_mmap_block(aligned_size);
        if (block) {
            block->free = 0;
            split_block(block, aligned_size);
        } else {
            block = request_space_mmap(aligned_size);
            if (!block) {
                return NULL;
            }
        }
    } else {
        block_meta* last = global_base;
        block = find_free_block(&last, aligned_size);
        if (!block) {
            block = request_space(last, aligned_size);
            if (!block) {
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

    return (block + 1);
}

void* my_calloc(size_t nmemb, size_t size) {
    size_t total_size;
    void* ptr;

    if (nmemb != 0 && size != 0) {
        if (nmemb > SIZE_MAX / size) {
            errno = ENOMEM;
            return NULL;
        }
    }

    total_size = nmemb * size;
    ptr = my_malloc(total_size);

    if (ptr) {
        memset(ptr, 0, total_size);
    }

    return ptr;
}

void my_free(void* ptr) {
    if (!ptr) {
        return;
    }

    block_meta* block_ptr = (block_meta*)ptr - 1;
    block_ptr->free = 1;

    if (block_ptr->mmaped) {
        coalesce_mmap(block_ptr);
    } else {
        // Coalescing for non-mmap'd blocks
        block_meta* current = global_base;
        while (current && current->next) {
            if (current->free && current->next->free) {
                current->size += current->next->size + sizeof(block_meta);
                current->next = current->next->next;
                if (current->next) {
                    current->next->prev = current;
                }
            } else {
                current = current->next;
            }
        }
    }
}