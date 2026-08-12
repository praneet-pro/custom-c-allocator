#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern void* my_malloc(size_t size);
extern void my_free(void* ptr);

#define ITERATIONS 500
#define MAX_ALLOC_SIZE 4096
#define MIN_ALLOC_SIZE 1

int main() {
    void* pointers_sys[ITERATIONS];
    void* pointers_custom[ITERATIONS];
    size_t random_sizes[ITERATIONS];
    clock_t start, end;
    double cpu_time_used;

    // Pre-generate random sizes to hit EVERY bin in your allocator
    srand(time(NULL));
    for (int i = 0; i < ITERATIONS; i++) {
        random_sizes[i] = (rand() % (MAX_ALLOC_SIZE - MIN_ALLOC_SIZE + 1)) + MIN_ALLOC_SIZE;
    }

    printf("Running Full-Spectrum Chaos Benchmark (1 to 4096 bytes)...\n");

    // --- SYSTEM MALLOC ---
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        pointers_sys[i] = malloc(random_sizes[i]);
    }
    for (int i = 0; i < ITERATIONS; i++) {
        free(pointers_sys[i]);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Standard malloc time: %f seconds\n", cpu_time_used);

    // --- CUSTOM MALLOC ---
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        pointers_custom[i] = my_malloc(random_sizes[i]);
    }
    for (int i = 0; i < ITERATIONS; i++) {
        my_free(pointers_custom[i]);
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Custom malloc time:   %f seconds\n", cpu_time_used);

    return 0;
}