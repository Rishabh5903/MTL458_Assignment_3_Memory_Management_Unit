#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define BLOCK_SIZE sizeof(block_t)

typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block_t;

static block_t *free_list = NULL;

static void *allocate_from_system(size_t size) {
    void *block = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void *my_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t aligned_size = ALIGN(size);
    block_t *block, *last = NULL;

    if ((block = find_free_block(&last, aligned_size))) {
        block->free = 0;
        split_block(block, aligned_size);
    } else {
        block = allocate_from_system(aligned_size + BLOCK_SIZE);
        if (!block) return NULL;

        block->size = aligned_size;
        block->free = 0;
        block->next = NULL;

        if (!free_list) {
            free_list = block;
        } else {
            last->next = block;
        }
    }

    return (void *)(block + 1);
}

void *my_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = my_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void my_free(void *ptr) {
    if (!ptr) return;

    block_t *block = (block_t *)ptr - 1;
    block->free = 1;

    coalesce();
}

void *my_realloc(void *ptr, size_t size) {
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
