#include "memory.h"

void *memset(void *ptr, unsigned char c, size_t num) {
    unsigned char *p = (unsigned char *)ptr;
    for (int i = 0; i < num; i++) {
        p[i] = c;
    }
    return ptr;
}