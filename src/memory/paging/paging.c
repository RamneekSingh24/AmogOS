#include "paging.h"
#include "memory/heap/kheap.h"
#include <stdbool.h>
#include "status.h"

void paging_load_directory(uint32_t* dir);

static uint32_t* current_directory = 0;

struct page_table_directory* create_paging_dir (uint8_t flags) {

    uint32_t* directory = kzmalloc(PAGING_TOTAL_ENTRIES_PER_TABLE * sizeof(uint32_t));
    
    int offset = 0;

    // 1024 page tables each with 1024 entries
    // each entry is 4KB page

    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {

        uint32_t* page_table = kzmalloc(PAGING_TOTAL_ENTRIES_PER_TABLE * sizeof(uint32_t));
        
        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
            page_table[j] = (offset + (j * PAGING_PAGE_SZ) ) | flags;
        }

        offset += PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SZ;
        directory[i] = (uint32_t) page_table | flags | PAGE_WRITEABLE;
        // All pages table entries in the directory[i] are writeable
    }

    struct page_table_directory* dir = kzmalloc(sizeof(struct page_table_directory));
    dir->page_tables = directory;

    return dir;

}

void paging_switch(uint32_t* dir) {
    paging_load_directory(dir);
    current_directory = dir;
}


uint32_t* get_page_tables(struct page_table_directory* dir) {
    return dir->page_tables;
}

bool is_aligned(void* addr) {
    return ((uint32_t) addr % PAGING_PAGE_SZ) == 0;
}



int get_indexes(void* virtual_address, uint32_t* page_table_index, 
                        uint32_t* page_table_entry_index) {

    int res = 0;
    if (!is_aligned(virtual_address)) {
        res = -STATUS_INVALID_ARG;
        return res;
    }

    *page_table_index = ((uint32_t) virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SZ));
    *page_table_entry_index = ((uint32_t) virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SZ) / PAGING_PAGE_SZ);


    return res;  
}



int set_page_table_entry(uint32_t* dir, void* virtual_addr, uint32_t val) {
    if (!is_aligned(virtual_addr)) {
        return -STATUS_INVALID_ARG;
    }
    uint32_t page_tabe_idx = 0;
    uint32_t page_table_entry_idx = 0;
    int res = get_indexes(virtual_addr, &page_tabe_idx, &page_table_entry_idx);
    if (res < 0) {
        return res;
    }

    uint32_t* table  = (uint32_t*)(dir[page_tabe_idx] & 0xfffff000);

    table[page_table_entry_idx] = val;

    return res;
}