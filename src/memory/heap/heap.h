#ifndef HEAP_H
#define HEAP_H

#include "../../config.h"
#include <stdint.h>
#include <stddef.h>

#define HEAP_BLOCK_TAKEN 0x01
#define HEAP_BLOCK_FREE 0x00


#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned char Heap_table_entry;

struct heap_table {
    Heap_table_entry* entries;
    size_t total;
};

struct heap {
    struct heap_table* table;
    void* start_addr;

};

int heap_init(struct heap* heap, void* mem_start_ptr, void* mem_end_ptr, struct heap_table* table);
void* heap_malloc(struct heap* heap, int size);
void heap_free(struct heap* heap, void* ptr);
#endif