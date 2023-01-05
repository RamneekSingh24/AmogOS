#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

enum {
    O_READ = 1,
    O_WRITE = 2,
    O_EXEC = 4,
};

void *malloc(size_t size);
void free(void *ptr);

int mmap(void *va_start, void *va_end, int flags);
int munmap(void *va_start);

void itoa(int value, char *buffer);
int atoi(char *buffer);

#endif