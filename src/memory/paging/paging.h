#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

/* Masks for toggling Page Table Entry bits */
#define PAGE_CACHE_DISABLE     0b00010000
#define PAGE_WRITE_THROUGH     0b00001000
#define PAGE_ACCESS_FROM_ALL   0b00000100   // Page accessible from user space
#define PAGE_WRITEABLE         0b00000010
#define PAGING_ENABLED         0b00000001


#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SZ 4096

// Two level paging
struct page_table_directory {
    uint32_t* page_tables;
};



struct page_table_directory* create_paging_dir (uint8_t flags);
void paging_switch(uint32_t* dir);
uint32_t* get_page_tables(struct page_table_directory* dir);


// Warning--
// Create page_table_dir and switch to a page table
// and then call enable paging to avoid panics
void enable_paging();


int set_page_table_entry(uint32_t* dir, void* virtual_addr, uint32_t val);

int get_indexes(void* virtual_address, uint32_t* page_table_index, 
                        uint32_t* page_table_entry_index);




#endif 