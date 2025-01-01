#include <dlfcn.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <bits/mman-linux.h>

#define MAP_ANONYMOUS 0x20

typedef struct Allocator {
    void *(*allocator_create)(void *addr, size_t size);
    void *(*allocator_alloc)(void *allocator, size_t size);
    void (*allocator_free)(void *allocator, void *ptr);
    void (*allocator_destroy)(void *allocator);
} Allocator;

void *standard_allocator_create(void *memory, size_t size) {
    (void)memory;
    (void)size;
    return memory;
}

void *standard_allocator_alloc(void *allocator, size_t size) {
    (void)allocator;
    uint32_t *memory = mmap(NULL, size + sizeof(uint32_t), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    *memory = (uint32_t)(size + sizeof(uint32_t)); 
    return memory + 1; 
}

void standard_allocator_free(void *allocator, void *memory) {
    (void)allocator;
    if (memory == NULL) return;
    uint32_t *mem = (uint32_t *)memory - 1; 
    munmap(mem, *mem); 
}

void standard_allocator_destroy(void *allocator) {
    (void)allocator;
}

void load_allocator(const char *library_path, Allocator *allocator) {
    if (library_path == NULL || library_path[0] == '\0') {
        allocator->allocator_create = standard_allocator_create;
        allocator->allocator_alloc = standard_allocator_alloc;
        allocator->allocator_free = standard_allocator_free;
        allocator->allocator_destroy = standard_allocator_destroy;
        return;
    }

    void *library = dlopen(library_path, RTLD_LOCAL | RTLD_NOW);
    if (!library) {
        fprintf(stderr, "WARNING: failed to load shared library: %s\n", dlerror());
        allocator->allocator_create = standard_allocator_create;
        allocator->allocator_alloc = standard_allocator_alloc;
        allocator->allocator_free = standard_allocator_free;
        allocator->allocator_destroy = standard_allocator_destroy;
        return;
    }

    allocator->allocator_create = dlsym(library, "allocator_create");
    allocator->allocator_alloc = dlsym(library, "allocator_alloc");
    allocator->allocator_free = dlsym(library, "allocator_free");
    allocator->allocator_destroy = dlsym(library, "allocator_destroy");

    if (!allocator->allocator_create || !allocator->allocator_alloc ||
        !allocator->allocator_free || !allocator->allocator_destroy) {
        fprintf(stderr, "Error: failed to load all allocator functions\n");
        dlclose(library);
        allocator->allocator_create = standard_allocator_create;
        allocator->allocator_alloc = standard_allocator_alloc;
        allocator->allocator_free = standard_allocator_free;
        allocator->allocator_destroy = standard_allocator_destroy;
    }
}

size_t round_up_to_power_of_two(size_t size) {
    if (size < 8) {
        return 8;
    }
    size_t power = 1;
    while (power < size) {
        power <<= 1;
    }
    return power;
}

int main(int argc, char **argv) {
    const char *library_path = (argc > 1) ? argv[1] : NULL;
    Allocator allocator_api;
    load_allocator(library_path, &allocator_api);

    size_t size = 4096; 
    size_t block_size = 16; 

    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        return EXIT_FAILURE;
    }

    void *allocator = allocator_api.allocator_create(addr, size);
    if (!allocator) {
        fprintf(stderr, "Failed to initialize allocator\n");
        munmap(addr, size);
        return EXIT_FAILURE;
    }

    void *blocks[12];
    size_t block_sizes[12] = {16, 16, 16, 32, 48, 64, 96, 128, 256, 128, 128, 128}; 

    int alloc_failed = 0;
    for (int i = 0; i < 12; ++i) {
        blocks[i] = allocator_api.allocator_alloc(allocator, block_sizes[i]);
        if (blocks[i] == NULL) {
            alloc_failed = 1;
            fprintf(stderr, "Memory allocation failed for block %d\n", i + 1);
            break;
        }
    }

    if (!alloc_failed) {
        printf("Memory allocated successfully\n");
        for (int i = 0; i < 12; ++i) {
            printf("Block %d address: %p\n", i + 1, blocks[i]);
        }
    }

    for (int i = 0; i < 12; ++i) {
        if (blocks[i] != NULL) {
            allocator_api.allocator_free(allocator, blocks[i]);
        }
    }
    printf("Memory freed\n");

    allocator_api.allocator_destroy(allocator);
    munmap(addr, size);

    printf("Program exited successfully\n");
    return EXIT_SUCCESS;
}