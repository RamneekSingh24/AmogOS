#include "memory.h"

void* memset(void* ptr, char c, size_t size) {
    char* itr = (char*) ptr;
    for (size_t i = 0; i < size; i++) {
        itr[i] = c;
    }
    return ptr;
}