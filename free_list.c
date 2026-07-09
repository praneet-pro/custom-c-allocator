#include <stdio.h>
#include <stddef.h>

typedef struct Block {
    size_t size;
    int is_free;
    struct Block* next;
    struct Block* prev;
} Block;

Block* free_list_head = NULL;

Block* find_free_block(size_t required_size) {
    Block* curr = free_list_head;

    while(curr != NULL) {
        if(curr->size >= required_size) 
            return curr;

        curr = curr->next;
    }
    return NULL;
}
