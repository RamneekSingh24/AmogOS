#include "stdlib.h"
#include "stdio.h"

// ----------------- Heap implementation start ------------ //

// A very very dirty malloc implementation for now
// Will come back and update this to use a free list like done in my OS class.

#define HEAP_TOP 0xc0000000
#define HEAP_INFO_MAGIC 198913
#define HEAP_ALLOC_CHUNK (1024 * 4) // 4 KB

unsigned int up_align(unsigned int va) {
    if (va % 4096 == 0) {
        return va;
    }
    return va + 4096 - va % 4096;
}

unsigned int down_align(unsigned int va) { return va - va % 4096; }

struct malloc_info {
    size_t size;
    int magic;
};

// grows down
size_t heap_end = HEAP_TOP;
size_t heap_curr = HEAP_TOP;

void *malloc(size_t size) {
    size_t alloc_size = size + sizeof(struct malloc_info);
    if (heap_curr - alloc_size < heap_end) {
        // need more space
        unsigned int new_end = down_align(heap_end - alloc_size);
        // Invariant: heap_end is always aligned
        int res = mmap((void *)(new_end), (void *)heap_end, O_READ | O_WRITE);
        if (res != 0) {
            return (void *)0;
        }
        heap_end = new_end;
    }
    // enough space
    heap_curr -= alloc_size;
    struct malloc_info *info = (struct malloc_info *)heap_curr;
    info->size = size;
    info->magic = HEAP_INFO_MAGIC;
    return (void *)(heap_curr + sizeof(struct malloc_info));
}

void free(void *ptr) {
    struct malloc_info *info =
        (struct malloc_info *)(ptr - sizeof(struct malloc_info));
    if (info->magic != HEAP_INFO_MAGIC) {
        return;
    }
    // TODO: free memory
    return;
}

// ----------------- Heap implementation end ------------ //

// ----------------- String functions start ------------------- //
#define MAX_DIGITS 12 // 32-bit int max digits (10)

void itoa(int value, char *buffer) {
    int idx = 0;
    if (value < 0) {
        buffer[idx++] = '-';
        value = -value;
    }
    int cx = value;
    int num_digits = 0;

    while (cx > 0) {
        num_digits++;
        cx /= 10;
    }
    if (num_digits == 0) {
        buffer[idx++] = '0';
        return;
    }
    char buf[MAX_DIGITS];
    for (int i = 0; i < num_digits; i++) {
        buf[i] = '0' + value % 10;
        value /= 10;
    }
    for (int i = num_digits - 1; i >= 0; i--) {
        buffer[idx++] = buf[i];
    }
}

int atoi(char *buffer) {
    int res = 0;
    int sign = 1;
    char bb[12];
    put_char('\n');
    itoa((int)buffer, bb);
    print(bb, 100);
    put_char('\n');
    print("bug??\n", 100);
    print("bug??2\n", 100);

    if (buffer[0] == 'A') {
        print("bug??3\n", 100);

        sign = -1;
        buffer++;
    }

    int idx = 0;
    while (buffer[idx] != 0 && idx < MAX_DIGITS) {
        put_char(idx + '0');
        put_char(buffer[idx]);
        put_char('\n');
        res *= 10;
        res += buffer[idx] - '0';
        idx++;
    }
    return res * sign;
}

// ----------------- String functions end ------------------- //

// ------------------- TESTS ------------------- //

void test_mmap_unmap() {
    unsigned int va_start = ((unsigned int)1024 * 1024 * 1024 * 3);
    unsigned int va_end = up_align(va_start + 4096 * 3 + 10);

    int res = mmap((void *)va_start, (void *)va_end, O_READ | O_WRITE);
    if (res != 0) {
        // print("mmap failed\n", 100);
        while (1) {
        };
    }
    // print("mmap success\n", 100);
    for (int i = 0; i < 10; i++) {
        *(char *)(va_start + i) = 'a' + i;
        if (i == 10) {
            *(char *)(va_start + i) = 0;
        }
    }
    // print((char*) va_start, 100);
    res = munmap((void *)va_start);
    if (res != 0) {
        // print("munmap failed\n", 100);
        while (1) {
        };
    }
    // print("munmap success\n", 100);

    *(char *)(va_start + 2) = 'T'; // should crash

    // char x = *(char*)(va_start+2);
    // put_char(x);
}