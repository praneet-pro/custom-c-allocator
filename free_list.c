#include <stdio.h>
#include <stddef.h>
#include <sys/mman.h>

#define HEAP_SIZE (1024 * 1024)

typedef struct Block {
    size_t size;        // holds both the size and free/alloc flag
    struct Block* next;
    struct Block* prev;
} Block;

// Bitwise Macros for O(1) inline speed
#define GET_SIZE(block) ((block)->size & ~1)
#define IS_FREE(block)  ((block)->size & 1)
#define SET_FREE(block) ((block)->size |= 1)
#define SET_ALLOC(block) ((block)->size &= ~1)
#define SET_SIZE_AND_FLAG(block, new_size, free_flag) ((block)->size = (new_size) | (free_flag))

static Block* segregated_lists[12] = {NULL};
static void* heap_start = NULL;

// To get index of the suitable bin in segregated list
int get_bin_index(size_t size) {
    if(size <= 128) {
        return (size >> 4) - 1;
    }

    else if(size <= 256) {
        return 8;
    }
    else if(size <= 512) {
        return 9;
    }
    else if(size <= 1024) {
        return 10;
    }

    else {
        return 11;
    }
}

// Searches for free block 
Block* find_free_block(size_t required_size) {
    int starting_index = get_bin_index(required_size);

    for(int i = starting_index; i < 12; i++) {
        Block* curr = segregated_lists[i];

        while(curr != NULL) {
            if(GET_SIZE(curr) >= required_size) {
                return curr;
            }
            curr = curr->next;
        }
    }

    return NULL;
}

// Inserts a block into the segregated list
void insert_block(Block* block) {
    size_t size = GET_SIZE(block);

    int index = get_bin_index(size);

    block->next = segregated_lists[index];
    block->prev = NULL;

    if(segregated_lists[index] != NULL) {
        segregated_lists[index]->prev = block;
    }

    segregated_lists[index] = block;
}


// Removes a block from segregated list
void remove_block(Block* block) {
    int size = GET_SIZE(block);
    int index = get_bin_index(size);

    if(block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        segregated_lists[index] = block->next;
    }

    if(block->next != NULL) {
        block->next->prev = block->prev;
    }

    block->next = NULL;
    block->prev = NULL;
}

// Splits the free block to prevent wasteage of memory
// Gives the required memory size
void split_block(Block* total_block, size_t required_size) {
    size_t total_size = GET_SIZE(total_block);

    // CASE 1: No split, Removing the block from free list
    if(total_size <= required_size + sizeof(Block) + sizeof(Block*)) {
        SET_ALLOC(total_block);
        return;
    }

    // CASE 2: Split
    size_t remaining_size = total_size - required_size - sizeof(Block) - sizeof(Block*);

    Block* new_block = (Block*)((char*)total_block + sizeof(Block) + required_size + sizeof(Block*));
    
    SET_SIZE_AND_FLAG(new_block, remaining_size, 1);

    Block** total_footer = (Block**)((char*)total_block + sizeof(Block) + required_size);
    *total_footer = total_block;

    Block** new_footer = (Block**)((char*)new_block + sizeof(Block) + remaining_size);
    *new_footer = new_block;

    SET_SIZE_AND_FLAG(total_block, required_size, 0);

    insert_block(new_block);
}

// Stiches the physically neighbouring free blocks together
void coalesce_blocks(Block* curr) {
    size_t curr_size = GET_SIZE(curr);
    // --- MERGE RIGHT ---
    Block* physical_right = (Block*)((char*)curr + sizeof(Block) + curr_size + sizeof(Block*));

    if ((char*)physical_right < ((char*)heap_start + HEAP_SIZE)) {
        if(IS_FREE(physical_right)) {
            remove_block(physical_right);

            size_t right_size = GET_SIZE(physical_right);
            size_t new_size = curr_size + sizeof(Block) + right_size + sizeof(Block*);

            SET_SIZE_AND_FLAG(curr, new_size, 1);

            Block** right_footer = (Block**)((char*)curr + sizeof(Block) + new_size);
            *right_footer = curr;

            curr_size = new_size;
        }
    }

    // --- MERGE LEFT ---
    if((void*)curr > heap_start) {
        Block** left_footer = (Block**)((char*)curr - sizeof(Block*));
        Block* physical_left = *left_footer;

        if(IS_FREE(physical_left)) {
            remove_block(physical_left);

            size_t left_size = GET_SIZE(physical_left);
            size_t new_size = left_size + sizeof(Block) + sizeof(Block*) + curr_size;

            SET_SIZE_AND_FLAG(physical_left, new_size, 1);

            Block** new_footer = (Block**)((char*)physical_left + sizeof(Block) + new_size);
            *new_footer = physical_left;

            curr = physical_left;
        }
    }

    insert_block(curr);
}

// frees the memory passed by the user(The user doesnot pass the header so we navigate to it)
void my_free(void *ptr) {
    if(ptr == NULL) return;

    Block* curr = (Block*)((char*)ptr - sizeof(Block));
    SET_FREE(curr);

    coalesce_blocks(curr);
}

// Gets 1MB of memory from OS and links to head for further usage
void init_heap() {
    if(heap_start != NULL) return;

    void* raw_memory = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(raw_memory == MAP_FAILED) {
        printf("System Error: mmap failed to allocate memory");
        return;
    }

    heap_start = raw_memory;

    Block* initial_block = (Block*)raw_memory;
    size_t initial_size = HEAP_SIZE - sizeof(Block) - sizeof(Block*);

    SET_SIZE_AND_FLAG(initial_block, initial_size, 1);

    Block** footer = (Block**)((char*)initial_block + sizeof(Block) + initial_size);
    *footer = initial_block;

    insert_block(initial_block);
}

// Gets the memory to the user
void* my_malloc(size_t size) {
    if(size == 0) return NULL;

    if(heap_start == NULL) {
        init_heap();
    }

    size_t alligned_size = (size + 15) & ~15;

    Block* found_block = find_free_block(alligned_size);
    if(found_block == NULL) {
        printf("Error: Out of memory.\n");
        return NULL;
    }

    remove_block(found_block);
    split_block(found_block, alligned_size);

    return (void*)((char*)found_block + sizeof(Block));
}
