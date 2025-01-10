#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h> 
#include <string.h>

#define MIN_BLOCK_SIZE 8
#define MAP_ANONYMOUS 0x20

typedef struct Allocator {
    void *memory;
    size_t size;
    void **free_lists;
    size_t num_levels;
} Allocator;

size_t round_up_pow2(size_t size) {
    size_t power = MIN_BLOCK_SIZE;
    while (power < size) {
        power *= 2;
    }
    return power;
}

size_t get_level(size_t block_size) {
    return (size_t)(log2(block_size) - log2(MIN_BLOCK_SIZE));
}

Allocator* allocator_create(void *const memory, const size_t size) {
    if (size < MIN_BLOCK_SIZE) {
        char exit_message[] = "Memory size too small\n";
        write(STDOUT_FILENO, exit_message, sizeof(exit_message) - 1);
        return NULL;
    }

    Allocator *allocator = (Allocator *)mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator == MAP_FAILED) {
        perror("mmap for allocator failed");
        return NULL;
    }

    allocator->memory = memory;
    allocator->size = size;
    allocator->num_levels = get_level(size) + 1;

    allocator->free_lists = (void **)mmap(NULL, allocator->num_levels * sizeof(void *), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator->free_lists == MAP_FAILED) {
        perror("mmap for free_lists failed");
        munmap(allocator, sizeof(Allocator));
        return NULL;
    }

    memset(allocator->free_lists, 0, allocator->num_levels * sizeof(void *));
    allocator->free_lists[allocator->num_levels - 1] = memory;
    return allocator;
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    if (!allocator || size > allocator->size) {
        return NULL;
    }

    size_t block_size = round_up_pow2(size);
    size_t level = get_level(block_size);

    for (size_t i = level; i < allocator->num_levels; i++) {
        if (allocator->free_lists[i] != NULL) {
            void *block = allocator->free_lists[i];
            allocator->free_lists[i] = *(void **)block;

            while (i > level) {
                i--;
                void *buddy = (void *)((char *)block + (1 << (i + (size_t)log2(MIN_BLOCK_SIZE))));                
                *(void **)buddy = allocator->free_lists[i];
                allocator->free_lists[i] = buddy;
            }

            return block;
        }
    }

    return NULL; 
}

void allocator_free(Allocator *const allocator, void *const memory) {
    if (!allocator || !memory) {
        return;
    }

    size_t offset = (char *)memory - (char *)allocator->memory;
    if (offset >= allocator->size) {
        return;
    }

    size_t level = 0;
    size_t block_size = MIN_BLOCK_SIZE;
    while (block_size < allocator->size && (offset % (block_size * 2)) == 0) {
        block_size *= 2;
        level++;
    }

    void *buddy = (void *)((char *)allocator->memory + (offset ^ block_size));
    void **current = &allocator->free_lists[level];
    while (*current) {
        if (*current == buddy) {
            *current = *(void **)(*current);
            allocator_free(allocator, (offset < (char *)buddy - (char *)allocator->memory) ? memory : buddy);
            return;
        }
        current = (void **)*current;
    }

    *(void **)memory = allocator->free_lists[level];
    allocator->free_lists[level] = memory;
}

void allocator_destroy(Allocator *const allocator) {
    if (!allocator) {
        return;
    }

    if (allocator->free_lists) {
        munmap(allocator->free_lists, allocator->num_levels * sizeof(void *));
    }
    munmap(allocator, sizeof(Allocator));
}
