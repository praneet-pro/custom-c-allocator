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

static Block* free_list_head = NULL;
static void* heap_start = NULL;

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
    // CASE 1: No split, Removing the block from free list
    if(total_block->size <= required_size + sizeof(Block) + sizeof(Block*)) {
        if(total_block->prev != NULL) {
            total_block->prev->next = total_block->next;
        } else {
            free_list_head = total_block->next;
        }

        if(total_block->next != NULL) {
            total_block->next->prev = total_block->prev;
        } 

        total_block->is_free = 0;
        return;
    }

    // CASE 2: Split
    size_t remaining_size = total_block->size - required_size - sizeof(Block) - sizeof(Block*);

    Block* new_block = (Block*)((char*)total_block + sizeof(Block) + required_size + sizeof(Block*));
    new_block->size = remaining_size;
    new_block->is_free = 1;

    Block** total_footer = (Block**)((char*)total_block + sizeof(Block) + required_size);
    *total_footer = total_block;

    Block** new_footer = (Block**)((char*)new_block + sizeof(Block) + remaining_size);
    *new_footer = new_block;

    new_block->next = total_block->next;
    new_block->prev = total_block->prev;

    if(new_block->prev != NULL) {
        new_block->prev->next = new_block;
    } else {
        free_list_head = new_block;
    }

    if(new_block->next != NULL) {
        new_block->next->prev = new_block;
    }

    total_block->size = required_size;
    total_block->is_free = 0;
}

// Stiches the physically neighbouring free blocks together
void coalesce_blocks(Block* curr) {
    // --- MERGE RIGHT ---
    Block* physical_right = (Block*)((char*)curr + sizeof(Block) + curr->size + sizeof(Block*));

    if ((char*)physical_right < ((char*)heap_start + HEAP_SIZE)) {
        if(physical_right->is_free) {
            curr->size = curr->size + sizeof(Block) + physical_right->size + sizeof(Block*);

            if(physical_right->prev != NULL) {
                physical_right->prev->next = physical_right->next;
            }
            if(physical_right->next != NULL) {
                physical_right->next->prev = physical_right->prev;
            }

            Block** right_footer = (Block**)((char*)curr + sizeof(Block) + curr->size);
            *right_footer = curr;
        }
    }

    // --- MERGE LEFT ---
    if((void*)curr > heap_start) {
        Block** left_footer = (Block**)((char*)curr - sizeof(Block*));

        Block* physical_left = *left_footer;

        if(physical_left->is_free) {
            physical_left->size = physical_left->size + sizeof(Block) + sizeof(Block*) + curr->size;

            Block** new_footer = (Block**)((char*)physical_left + sizeof(Block) + physical_left->size);
            *new_footer = physical_left;

            curr = physical_left;
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

    heap_start = raw_memory;

    free_list_head->size = HEAP_SIZE - sizeof(Block) - sizeof(Block*);
    free_list_head->is_free = 1;
    free_list_head->next = NULL;
    free_list_head->prev = NULL;

    Block** footer = (Block**)((char*)free_list_head + sizeof(Block) + free_list_head->size);
    *footer = free_list_head;
}

// Gets the memory to the user
void* my_malloc(size_t size) {
    if(size == 0) return NULL;

    if(free_list_head == NULL) {
        init_heap();
    }

    size_t alligned_size = (size + 15) & ~15;

    Block* found_block = find_free_block(alligned_size);

    if(found_block == NULL) {
        printf("Error: Out of memory.\n");
        return NULL;
    }

    split_block(found_block, alligned_size);

    return (void*)((char*)found_block + sizeof(Block));
}