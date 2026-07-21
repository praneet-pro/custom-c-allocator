#ifndef FREE_LIST_H
#define FREE_LIST_H

#include <stddef.h>

// Function prototypes
void* my_malloc(size_t size);
void my_free(void* ptr);

#endif