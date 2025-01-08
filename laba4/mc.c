#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>

#define BLOCK_SIZE 512
#define TOTAL_BLOCKS 1000
#define NUM_LISTS 10  
#define MAP_ANONYMOUS 0x20

typedef struct Block {
    uint32_t index;           
    bool is_free;             
    size_t size;              
    struct Block* next;       
    struct Block* prev;       
} Block;

typedef struct Allocator {
    Block* blocks;            
    Block* free_lists[NUM_LISTS]; 
    uint32_t total_blocks;    
} Allocator;

int get_list_index(size_t size) {
    int index = 0;
    while (size > BLOCK_SIZE) {
        size /= 2;
        index++;
    }
    return index;
}

Block* get_previous_block(Allocator* allocator, Block* block) {
    if (block->index > 0) {
        return &allocator->blocks[block->index - 1];
    }
    return NULL;
}

Block* get_next_block(Allocator* allocator, Block* block) {
    if (block->index < allocator->total_blocks - 1) {
        return &allocator->blocks[block->index + 1];
    }
    return NULL;
}

void merge_blocks(Allocator* allocator, Block* block1, Block* block2) {
    block1->size += block2->size;
    block1->next = block2->next;
    if (block2->next) {
        block2->next->prev = block1;
    }
    block2->is_free = false;  
}

Allocator* allocator_create() {
    Allocator* allocator = (Allocator*)mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator == MAP_FAILED) {
        perror("mmap for allocator failed");
        return NULL;
    }

    allocator->total_blocks = TOTAL_BLOCKS;

    allocator->blocks = (Block*)mmap(NULL, allocator->total_blocks * sizeof(Block), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator->blocks == MAP_FAILED) {
        perror("mmap for blocks failed");
        munmap(allocator, sizeof(Allocator));
        return NULL;
    }

    for (int i = 0; i < NUM_LISTS; i++) {
        allocator->free_lists[i] = NULL;
    }

    for (uint32_t i = 0; i < allocator->total_blocks; i++) {
        allocator->blocks[i].index = i;
        allocator->blocks[i].is_free = true;
        allocator->blocks[i].size = BLOCK_SIZE;  
        allocator->blocks[i].next = NULL;
        allocator->blocks[i].prev = NULL;

        int list_index = get_list_index(allocator->blocks[i].size);
        if (allocator->free_lists[list_index]) {
            allocator->free_lists[list_index]->prev = &allocator->blocks[i];
        }
        allocator->blocks[i].next = allocator->free_lists[list_index];
        allocator->free_lists[list_index] = &allocator->blocks[i];
    }

    return allocator;
}

Block* allocator_alloc(Allocator* allocator, size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    int list_index = get_list_index(size);
    while (list_index < NUM_LISTS && !allocator->free_lists[list_index]) {
        list_index++;
    }

    if (list_index >= NUM_LISTS) {
        return NULL;  
    }

    Block* block = allocator->free_lists[list_index];
    block->is_free = false;

    allocator->free_lists[list_index] = block->next;
    if (allocator->free_lists[list_index]) {
        allocator->free_lists[list_index]->prev = NULL;
    }

    return block;
}

void allocator_free(Allocator* allocator, Block* block) {
    if (!allocator || !block || block->is_free) {
        return;
    }

    block->is_free = true;

    Block* prev_block = get_previous_block(allocator, block);
    Block* next_block = get_next_block(allocator, block);

    if (prev_block && prev_block->is_free) {
        merge_blocks(allocator, prev_block, block);
        block = prev_block;
    }

    if (next_block && next_block->is_free) {
        merge_blocks(allocator, block, next_block);
    }

    int list_index = get_list_index(block->size);
    block->next = allocator->free_lists[list_index];
    block->prev = NULL;
    if (allocator->free_lists[list_index]) {
        allocator->free_lists[list_index]->prev = block;
    }
    allocator->free_lists[list_index] = block;
}

void allocator_destroy(Allocator* allocator) {
    if (allocator) {
        if (allocator->blocks) {
            munmap(allocator->blocks, allocator->total_blocks * sizeof(Block));
        }
        munmap(allocator, sizeof(Allocator));
    }
}
