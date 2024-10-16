#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define META_SIZE ALIGN(sizeof(struct block_meta))
#define MMAP_THRESHOLD 131072 // mmap for large allocations (> 128KB)

struct block_meta {
    size_t size;
    int is_mmap;
    struct block_meta *next;
    int free;
};

static struct block_meta *head = NULL;

static struct block_meta *find_free_block(size_t size) {
    struct block_meta *current = head;
    while (current) {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    return NULL;
}

static void split_block(struct block_meta *block, size_t size) {
    if (block->size >= size + META_SIZE + ALIGNMENT) {
        struct block_meta *new_block = (void*)((char*)block + META_SIZE + size);
        new_block->size = block->size - size - META_SIZE;
        new_block->is_mmap = 0;
        new_block->free = 1;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

static void *allocate_with_mmap(size_t size) {
    size_t total_size = ALIGN(size + META_SIZE);
    if (total_size < MMAP_THRESHOLD) {
        total_size = MMAP_THRESHOLD;
    }
    void *ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    struct block_meta *block = ptr;
    block->size = total_size - META_SIZE;
    block->is_mmap = 1;
    block->free = 0;
    block->next = NULL;
    return block;
}

void* my_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t aligned_size = ALIGN(size);
    struct block_meta *block;

    if (aligned_size + META_SIZE >= MMAP_THRESHOLD) {
        // Use mmap for large allocations
        block = allocate_with_mmap(aligned_size);
        if (!block) return NULL;
    } else {
        block = find_free_block(aligned_size);
        if (!block) {
            // No free block found, allocate new memory
            block = sbrk(0);
            void *request = sbrk(aligned_size + META_SIZE);
            if (request == (void*) -1) {
                return NULL;
            }
            block->size = aligned_size;
            block->is_mmap = 0;
            block->next = NULL;
            block->free = 0;
        } else {
            // Found a free block
            block->free = 0;
            split_block(block, aligned_size);
        }
    }

    if (!head) head = block;

    return (block + 1);
}

void* my_calloc(size_t nelem, size_t size) {
    size_t total_size = nelem * size;
    void *ptr = my_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void my_free(void* ptr) {
    if (!ptr) return;

    struct block_meta *block_ptr = (struct block_meta*)ptr - 1;

    if (block_ptr->is_mmap) {
        // Free memory allocated with mmap
        munmap(block_ptr, block_ptr->size + META_SIZE);
    } else {
        // Mark the block as free
        block_ptr->free = 1;

        // Coalesce with next block if it's free
        if (block_ptr->next && block_ptr->next->free) {
            block_ptr->size += block_ptr->next->size + META_SIZE;
            block_ptr->next = block_ptr->next->next;
        }

        // Coalesce with previous block if it's free
        struct block_meta *current = head;
        while (current && current->next) {
            if (current->free && (void*)((char*)current + current->size + META_SIZE) == block_ptr) {
                current->size += block_ptr->size + META_SIZE;
                current->next = block_ptr->next;
                break;
            }
            current = current->next;
        }
    }
}
