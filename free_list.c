#include <stdio.h>
#include <stddef.h>

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