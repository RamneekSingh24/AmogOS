#include "paging.h"
#include "config.h"
#include "memory/heap/kheap.h"

struct page_table_32b kpage_table;

struct page_table_32b *current_pt[N_CPU_MAX];

extern void paging_load_dir(uint32_t *cr3);
extern void paging_enable();

void kpaging_create(uint8_t flags, struct page_table_32b *page_table) {
    page_table_entry *pt_dir =
        kzalloc(sizeof(page_table_entry) * NUM_PAGE_TABLE_ENTRIES);
    int offset = 0;
    for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
        page_table_entry *pt_i =
            kzalloc(sizeof(page_table_entry) * NUM_PAGE_TABLE_ENTRIES);
        for (int j = 0; j < NUM_PAGE_TABLE_ENTRIES; j++) {
            pt_i[j] = (offset + j * PAGE_SIZE) | flags;
        }

        offset += PAGE_SIZE * NUM_PAGE_TABLE_ENTRIES;
        uint32_t pt_i_frame_num = (uint32_t)(pt_i) / PAGE_SIZE;
        pt_dir[i] = (pt_i_frame_num * PAGE_SIZE) | flags | PAGE_WRITE_ALLOW;
    }

    page_table->cr3 = pt_dir;
    page_table->num_levels = 2;
}

void paging_switch(struct page_table_32b *pt) {
    // int cpu = get_cpu_id();
    int cpu = 0;

    paging_load_dir(pt->cr3);

    current_pt[cpu] = pt;
}

void kpaging_init() {
    kpaging_create(PAGE_PRESENT | PAGE_WRITE_ALLOW, &kpage_table);
    paging_switch(&kpage_table);
    paging_enable();
}
