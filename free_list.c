#include <stdio.h>
#include <stddef.h>
#include <sys/mman.h>

#define HEAP_SIZE (1024 * 1024)

typedef struct Block {
    size_t size;
    int is_free;
    struct Block* next;
    struct Block* prev;
} Block;

Block* free_list_head = NULL;

// Searches for free block 
Block* find_free_block(size_t required_size) {
    Block* curr = free_list_head;

    while(curr != NULL) {
        if(curr->size >= required_size) 
            return curr;

        curr = curr->next;
    }
    return NULL;
}


// Splits the free block to prevent wasteage of memory
// Gives the required memory size
void split_block(Block* total_block, size_t required_size) {
    size_t remaining_size = total_block->size - required_size - sizeof(Block);

    if(remaining_size <= sizeof(Block)) {
        return;
    }

    Block* new_block = (Block*)((char*)total_block + sizeof(Block) + required_size);

    new_block->size = remaining_size;
    new_block->is_free = 1;

    new_block->next = total_block->next;
    new_block->prev = total_block;

    if(total_block->next != NULL) {
        total_block->next->prev = new_block;
    }

    total_block->next = new_block;
    total_block->size = required_size;
    total_block->is_free = 0;
}

// Stiches the physically neighbouring free blocks together
void coalesce_blocks(Block* curr) {
    Block* physical_right = (Block*)((char*)curr + sizeof(Block) + curr->size);

    if(physical_right->is_free) {
        curr->size = curr->size + sizeof(Block) + physical_right->size;

        if(physical_right->prev != NULL) {
            physical_right->prev->next = physical_right->next;
        }
        if(physical_right->next != NULL) {
            physical_right->next->prev = physical_right->prev;
        }
    }
}

// frees the memory passed by the user(The user doesnot pass the header so we navigate to it)
void my_free(void *ptr) {
    if(ptr == NULL) return;

    Block* curr = (Block*)((char*)ptr - sizeof(Block));
    curr->is_free = 1;

    curr->next = free_list_head;
    curr->prev = NULL;

    if(free_list_head != NULL) {
        free_list_head->prev = curr;
    }

    free_list_head = curr;

    coalesce_blocks(curr);
}

// Gets 1MB of memory from OS and links to head for further usage
void init_heap() {
    if(free_list_head != NULL) return;

    void* raw_memory = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(raw_memory == MAP_FAILED) {
        printf("System Error: mmap failed to allocate memory");
        return;
    }

    free_list_head = (Block*)raw_memory;

    free_list_head->size = HEAP_SIZE - sizeof(Block);
    free_list_head->is_free = 1;
    free_list_head->next = NULL;
    free_list_head->prev = NULL;
}

// Gets the memory to the user
void* my_malloc(size_t size) {
    if(size == 0) return NULL;

    if(free_list_head == NULL) {
        init_heap();
    }

    Block* found_block = find_free_block(size);

    if(found_block == NULL) {
        printf("Error: Out of memory.\n");
        return NULL;
    }

    split_block(found_block, size);

    return (void*)((char*)found_block + sizeof(Block));
}


int main() {
    printf("--- BOOTING CUSTOM ALLOCATOR ---\n\n");

    // 1. Allocate a string
    printf("[1] Allocating 50 bytes for a string...\n");
    char* name = (char*)my_malloc(50);
    
    if (name != NULL) {
        printf("    SUCCESS! Payload address: %p\n", (void*)name);
        // Let's prove we can write to it without crashing
        sprintf(name, "Linus Torvalds");
        printf("    Data stored: %s\n\n", name);
    }

    // 2. Allocate an array of integers
    printf("[2] Allocating 100 bytes for an integer array...\n");
    int* numbers = (int*)my_malloc(100);
    
    if (numbers != NULL) {
        printf("    SUCCESS! Payload address: %p\n", (void*)numbers);
        numbers[0] = 42;
        printf("    Data stored: %d\n\n", numbers[0]);
    }

    // 3. Prove the Chainsaw worked (Pointer Math)
    // If the 32-byte header exists, the distance between the two payload addresses
    // should be exactly: 50 bytes (first payload) + 32 bytes (second header) = 82 bytes!
    printf("[3] Checking the physical layout...\n");
    size_t distance = (char*)numbers - (char*)name;
    printf("    Distance between allocations: %zu bytes\n\n", distance);

    // 4. Free the memory (Triggering coalescing)
    printf("[4] Freeing memory...\n");
    my_free(name);
    printf("    Freed 'name'.\n");
    my_free(numbers);
    printf("    Freed 'numbers'.\n\n");

    printf("--- SYSTEM SHUTDOWN CLEAN ---\n");
    return 0;
}