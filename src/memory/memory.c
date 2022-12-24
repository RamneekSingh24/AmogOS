#include "memory.h"

void *memset(void *ptr, unsigned char c, size_t num) {
    unsigned char *p = (unsigned char *)ptr;
    for (int i = 0; i < num; i++) {
        p[i] = c;
    }
    return ptr;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    unsigned char *p1 = (unsigned char *)ptr1;
    unsigned char *p2 = (unsigned char *)ptr2;
    for (int i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

void memcpy(void *dest, const void *src, size_t num) {
    unsigned char *p1 = (unsigned char *)dest;
    unsigned char *p2 = (unsigned char *)src;
    for (int i = 0; i < num; i++) {
        p1[i] = p2[i];
    }
}