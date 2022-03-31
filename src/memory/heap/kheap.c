#include "kheap.h"
#include "heap.h"
#include "../../config.h"
#include "../../kernel.h"
#include "memory/memory.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;


void kheap_init() {
    int NUM_ENTRIES = HEAP_SIZE_BYTES / HEAP_BLOCK_SZ;
    kernel_heap_table.entries = (Heap_table_entry *) HEAP_TABLE_ADDR;
    kernel_heap_table.total = NUM_ENTRIES;

    void* end = (void*)(HEAP_START + HEAP_SIZE_BYTES);
    int res = heap_init(&kernel_heap, (void*)(HEAP_START), end, &kernel_heap_table);
    if (res < 0) {
        print("Failed to create heap\n");
    }

}


void* kmalloc(size_t size) {
    return heap_malloc(&kernel_heap, size);
}

void* kzmalloc(size_t size) {
    void* ptr = kmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}



void kfree(void* ptr) {
    heap_free(&kernel_heap, ptr);
}