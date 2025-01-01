#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h> 

#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 1000
#define MAP_ANONYMOUS 0x20

typedef struct Block {
    uint32_t index;           
    bool is_free;           
    struct Block* next;     
    struct Block* prev;    
} Block;

typedef struct McKusickKarelsAllocator {
    Block* blocks;          
    Block* free_list;        
    uint32_t total_blocks;  
} McKusickKarelsAllocator;

McKusickKarelsAllocator* allocator_create() {
    McKusickKarelsAllocator* allocator = (McKusickKarelsAllocator*)mmap(
        NULL, sizeof(McKusickKarelsAllocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator == MAP_FAILED) {
        perror("mmap for allocator failed");
        return NULL;
    }

    allocator->total_blocks = TOTAL_BLOCKS;

    allocator->blocks = (Block*)mmap(
        NULL, allocator->total_blocks * sizeof(Block), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator->blocks == MAP_FAILED) {
        perror("mmap for blocks failed");
        munmap(allocator, sizeof(McKusickKarelsAllocator));
        return NULL;
    }

    allocator->free_list = NULL;
    for (uint32_t i = 0; i < allocator->total_blocks; i++) {
        allocator->blocks[i].index = i;
        allocator->blocks[i].is_free = true;
        allocator->blocks[i].next = allocator->free_list;
        allocator->blocks[i].prev = NULL;

        if (allocator->free_list) {
            allocator->free_list->prev = &allocator->blocks[i];
        }
        allocator->free_list = &allocator->blocks[i];
    }

    return allocator;
}

Block* allocator_alloc(McKusickKarelsAllocator* allocator) {
    if (!allocator || !allocator->free_list) {
        return NULL;
    }

    Block* block = allocator->free_list;
    block->is_free = false;

    allocator->free_list = block->next;
    if (allocator->free_list) {
        allocator->free_list->prev = NULL;
    }

    return block;
}

void allocator_free(McKusickKarelsAllocator* allocator, Block* block) {
    if (allocator && block && !block->is_free) {
        block->is_free = true;

        block->next = allocator->free_list;
        block->prev = NULL;

        if (allocator->free_list) {
            allocator->free_list->prev = block;
        }
        allocator->free_list = block;
    }
}

void allocator_destroy(McKusickKarelsAllocator* allocator) {
    if (allocator) {
        if (allocator->blocks) {
            munmap(allocator->blocks, allocator->total_blocks * sizeof(Block));
        }
        munmap(allocator, sizeof(McKusickKarelsAllocator));
    }
}
