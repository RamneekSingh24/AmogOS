#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *memset(void *ptr, unsigned char c, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void memcpy(void *dest, const void *src, size_t num);

#endif