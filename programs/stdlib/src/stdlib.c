#include "include/stdlib.h"
#include "include/os.h"
#include "include/stdio.h"
#include "include/string.h"
#include <stdarg.h>

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
#define MAX_DIGITS 13 // 32-bit int max digits (10)

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
        buffer[idx] = 0;
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
    buffer[idx] = 0;
}

int atoi(char *buffer) {
    int res = 0;
    int sign = 1;

    if (buffer[0] == '-') {
        sign = -1;
        buffer++;
    }

    int idx = 0;
    while (buffer[idx] != 0 && idx < MAX_DIGITS) {
        res *= 10;
        res += buffer[idx] - '0';
        idx++;
    }
    return res * sign;
}

int strlen(const char *str) {
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

int strnlen(const char *str, int maxlen) {
    int i = 0;
    while (str[i] != '\0' && i < maxlen) {
        i++;
    }
    return i;
}

char *strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

char *strncpy(char *dst, const char *src, int maxlen) {
    int i = 0;
    while (i < maxlen && src[i]) {
        dst[i] = src[i];
        i++;
    }
    if (i == maxlen && i > 0) {
        dst[i - 1] = '\0';
    } else if (i < maxlen) {
        dst[i] = '\0';
    } else if (i == 0 && maxlen > 0) {
        dst[i] = '\0';
    }
    return dst;
}

int strncmp(const char *str1, const char *str2, int maxlen) {
    int i = 0;
    while (i < maxlen && str1[i] && str2[i]) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        i++;
    }
    if (i == maxlen) {
        return 0;
    } else if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    } else if (str1[i] == '\0') {
        return -1;
    } else {
        return 1;
    }
}

int strnlen_terminator(const char *str, int maxlen, char terminator) {
    int i = 0;
    while (i < maxlen && str[i] != '\0' && str[i] != terminator) {
        i++;
    }
    return i;
}

char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

int istrncmp(const char *str1, const char *str2, int maxlen) {
    int i = 0;
    while (i < maxlen && str1[i] && str2[i]) {
        if (to_lower(str1[i]) != to_lower(str2[i])) {
            return str1[i] - str2[i];
        }
        i++;
    }
    if (i == maxlen) {
        return 0;
    } else if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    } else if (str1[i] == '\0') {
        return -1;
    } else {
        return 1;
    }
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }

void str_to_lower(char *str) {
    int i = 0;
    while (str[i]) {
        str[i] = to_lower(str[i]);
        i++;
    }
}

// FIXME: [BUG FIXED, loaded segment properly when mem_sz > file_sz]

// If a global variable is set to 0(or not set to anything) then we are seeing
// that it is not being inited when we run the program But if it is set to non
// zero then we see the correct init value

// probably variables explicitly initialized to 0 are not kept in the elf file
// but only a space is specified(see .bss section) without any initial values.
// Perhaps the kernel is not initializing the .bss section correctly[Indeed it
// wasn't]
char *sp = 0;

// Not thread safe
char *strtok(char *str, const char *delimiters) {
    int i = 0;
    int len = strlen(delimiters);
    if (!str && !sp) {
        return 0;
    }

    if (str && !sp) {
        sp = str;
    }
    char *p_start = sp;
    while (1) {
        for (i = 0; i < len; i++) {
            if (*p_start == delimiters[i]) {
                p_start++;
                break;
            }
        }

        if (i == len) {
            sp = p_start;
            break;
        }
    }

    if (*sp == '\0') {
        sp = 0;
        return sp;
    }

    // Find end of substring
    while (*sp != '\0') {
        for (i = 0; i < len; i++) {
            if (*sp == delimiters[i]) {
                *sp = '\0';
                break;
            }
        }

        sp++;
        if (i < len)
            break;
    }

    return p_start;
}

// ----------------- String functions end ------------------- //

// ---------------- Stdio functions start ------------------ //

static void ptr_to_str(void *ptr, char *out) {
    int i = 0;
    int j = 0;
    char buf[16];
    unsigned int val = (unsigned int)ptr;
    while (val) {
        buf[i++] = (val % 16) + '0';
        if (buf[i - 1] > '9') {
            buf[i - 1] += 7;
        }
        val /= 16;
    }
    if (i == 0) {
        buf[i++] = '0';
    }
    out[j++] = '0';
    out[j++] = 'x';
    while (i > 0) {
        out[j++] = buf[i - 1];
        i--;
    }
    out[j] = '\0';
}

// Unsafe function
// Need fmt to be null terminated
// Otherwise program might crash
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int idx = 0;
    const int bufsz = MAX_DIGITS + 12;
    char buf[bufsz];
    int ival;
    while (fmt[idx] != 0) {
        if (fmt[idx] == '%') {
            idx++;
            if (fmt[idx] == 'd') {
                ival = va_arg(args, int);
                itoa(ival, buf);
                print(buf, bufsz);
            } else if (fmt[idx] == 's') {
                char *str = va_arg(args, char *);
                // strlen doesn't count null terminator
                print(str, strlen(str) + 1);
            } else if (fmt[idx] == 'c') {
                char c = va_arg(args, int);
                put_char(c);
            } else if (fmt[idx] == 'p') {
                void *ptr = va_arg(args, void *);
                ptr_to_str(ptr, buf);
                print(buf, bufsz);
            }
        } else {
            put_char(fmt[idx]);
        }
        idx++;
    }
    va_end(args);
    return 0;
}

static char get_key_blocking() {
    char c;
    while (1) {
        c = get_key();
        if (c != 0) {
            return c;
        }
    }
}

// returns line read while also printing it to the screen
void readline_terminal(char *buf, int max_len) {
    int idx = 0;
    while (idx < max_len) {
        char key = get_key_blocking();
        if (key == 0x0d) {
            // ENTER key
            put_char('\n');
            buf[idx] = 0;
            return;
        } else if (key == 0x08) {
            // BACKSPACE key
            if (idx > 0) {
                buf[--idx] = 0;
                put_char(key);
            }
        } else {
            put_char(key);
            buf[idx++] = key;
        }
    }
}

// ---------------- Stdio functions End ------------------ //

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