#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "free_list.h" 

#define ITERATIONS 1000000
#define ARRAY_SIZE 256
#define MAX_ALLOC_SIZE 1024

int main() {
    clock_t start, end;
    double cpu_time_used;

    void* standard_blocks[ARRAY_SIZE] = {NULL};
    void* custom_blocks[ARRAY_SIZE] = {NULL};

    srand(time(NULL));

    // 1. STANDARD MALLOC CHAOS TEST
    start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int index = rand() % ARRAY_SIZE;
        
        if (standard_blocks[index] != NULL) {
            // Slot is full: Free it to create fragmentation
            free(standard_blocks[index]);
            standard_blocks[index] = NULL;
        } else {
            // Slot is empty: Allocate a random size
            size_t size = (rand() % MAX_ALLOC_SIZE) + 1;
            standard_blocks[index] = malloc(size);
            
            // Anti-lazy OS trick
            if (standard_blocks[index] != NULL) {
                ((char*)standard_blocks[index])[0] = 'A';
            }
        }
    }
    
    // Clean up any blocks left in the array at the end
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (standard_blocks[i] != NULL) {
            free(standard_blocks[i]);
        }
    }
    
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Standard malloc chaos time: %f seconds\n", cpu_time_used);

    // 2. CUSTOM MALLOC CHAOS TEST
    start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int index = rand() % ARRAY_SIZE;
        
        if (custom_blocks[index] != NULL) {
            my_free(custom_blocks[index]);
            custom_blocks[index] = NULL;
        } else {
            size_t size = (rand() % MAX_ALLOC_SIZE) + 1;
            custom_blocks[index] = my_malloc(size);
            
            if (custom_blocks[index] != NULL) {
                ((char*)custom_blocks[index])[0] = 'A';
            }
        }
    }
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (custom_blocks[i] != NULL) {
            my_free(custom_blocks[i]);
        }
    }
    
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Custom malloc chaos time: %f seconds\n", cpu_time_used);

    return 0;
}