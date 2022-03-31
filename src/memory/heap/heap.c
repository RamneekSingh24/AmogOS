#include "heap.h"
#include "kernel.h"
#include "../../status.h"
#include "../memory.h"

static int is_aligned(void *ptr) {
    return ((unsigned int)ptr % HEAP_BLOCK_SZ == 0);
}

static int table_is_valid(void* start, void* end, struct heap_table* table) {
    int res = 0;
    size_t table_size = (size_t) (end - start);
    size_t total_blocks = table_size / HEAP_BLOCK_SZ;
    if (table->total != total_blocks) {
        res = -STATUS_INVALID_ARG;
    }
    return res;
}

int heap_init(struct heap* heap, void* mem_start_ptr, void* mem_end_ptr, struct heap_table* table) {
    
    int res = 0;

    if (!is_aligned(mem_start_ptr) || !is_aligned(mem_end_ptr)) {
        res = -STATUS_INVALID_ARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->start_addr = mem_start_ptr;
    heap->table = table;


    res = table_is_valid(mem_start_ptr, mem_end_ptr, table);
    if (res < 0) {
        goto out;
    }
    
    size_t table_size = sizeof(Heap_table_entry) * table->total;
    memset(table->entries, HEAP_BLOCK_FREE, table_size);

out:
    return res;
}



void* heap_malloc(struct heap* heap, int size) {

    int req_blocks = (size + (HEAP_BLOCK_SZ - 1)) / HEAP_BLOCK_SZ;

    void* rtn_addr = 0;

    if (req_blocks <= 0) {
        return rtn_addr;
    }

    // Find consucutive blocks

    struct heap_table* table = heap->table;
    int found_blocks = 0;


    int i = 0, sb = 0, eb = 0;

    for (i = 0; i < table->total; i++) {
        if ((table->entries[i] & 0x0f) != HEAP_BLOCK_FREE) {
            found_blocks = 0;
            continue;
        }
        found_blocks++;
        if (found_blocks == req_blocks) {
            sb = (i - req_blocks + 1);
            rtn_addr = heap->start_addr + (HEAP_BLOCK_SZ * sb);
            break;
        }
    }

    // Mark Blocks as taken

    if (rtn_addr != NULL) {
        eb = sb + found_blocks - 1;
        table->entries[sb] = HEAP_BLOCK_TAKEN | HEAP_BLOCK_IS_FIRST | HEAP_BLOCK_HAS_NEXT;
        for (i = sb + 1; i < eb; i++) {
            table->entries[i] = HEAP_BLOCK_TAKEN | HEAP_BLOCK_HAS_NEXT;
        }
        if (eb > sb) table->entries[eb] = HEAP_BLOCK_TAKEN;
    }

    return rtn_addr;
}
void heap_free(struct heap* heap, void* ptr) {
    int start_block = ((int)(ptr - heap->start_addr)) / HEAP_BLOCK_SZ;
    struct heap_table* table = heap->table;

    for (int i = start_block; i < (int) table->total; i++) {
        int is_last = table->entries[i] & HEAP_BLOCK_HAS_NEXT;
        table->entries[i] = HEAP_BLOCK_FREE;
        if (is_last) {
            break;
        }
    }

}