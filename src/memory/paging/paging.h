#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_CACHE_DISABLED 0b00010000      // Page won't be cached
#define PAGE_CACHE_WRITE_THROUGH 0b00001000 // Page won't be cached
#define PAGE_USER_ACCESS_ALLOW                                                 \
    0b00000100                      // Page can be accessed in all ring levels
#define PAGE_WRITE_ALLOW 0b00000010 // Page can be written to
#define PAGE_PRESENT 0b00000001     // Page is present

#define PAGE_FRAME_LOC_MASK 0xFFFFF000

#define NUM_PAGE_TABLE_ENTRIES 1024

typedef uint32_t page_table_entry;

struct page_table_32b {
    page_table_entry *cr3;
    int num_levels; // 1 for large pages(4 MB), 2 for normal 4KB in 32 bit mode.
};

void kpaging_init();
void paging_load_kernel_page_table();
int paging_create_4gb_page_tables(uint8_t flags, struct page_table_32b *pt);
int paging_free_page_table(struct page_table_32b *table_table);
void paging_switch(struct page_table_32b *pt);

void *paging_down_align_addr(void *addr);
void *paging_up_align_addr(void *addr);

int paging_map_memory_region(struct page_table_32b *pt, void *data_va_start,
                             void *data_paddr, void *data_va_end,
                             uint8_t flags);

void test_paging_set();

int paging_free_va(struct page_table_32b *pt, uint32_t vaddr_start,
                   uint32_t vaddr_end);

#endif