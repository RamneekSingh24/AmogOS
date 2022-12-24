#include "paging.h"
#include "config.h"
#include "console/console.h"
#include "memory/heap/kheap.h"
#include "status.h"

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

bool is_page_aligned(uint32_t addr) { return (addr & (PAGE_SIZE - 1)) == 0; }

int paging_map_page(struct page_table_32b *pt, uint32_t virt_addr,
                    uint32_t phys_addr, uint8_t flags) {
    if (pt->num_levels != 2) {
        return -STATUS_INVALID_ARG;
    }

    if (!is_page_aligned(virt_addr) || !is_page_aligned(phys_addr)) {
        return -STATUS_INVALID_ARG;
    }

    uint32_t *pt_dir = pt->cr3;
    uint32_t pt_dir_index = virt_addr / (PAGE_SIZE * NUM_PAGE_TABLE_ENTRIES);
    uint32_t pt_index =
        (virt_addr % (PAGE_SIZE * NUM_PAGE_TABLE_ENTRIES)) / PAGE_SIZE;

    uint32_t *pt_i = (uint32_t *)(pt_dir[pt_dir_index] & PAGE_FRAME_NUM_MASK);
    pt_i[pt_index] = phys_addr | flags;

    return 0;
}

// -------------------- Tests -------------------- //

void test_paging_set() {
    int *ptr_pa = (int *)kzalloc(4096);
    paging_map_page(&kpage_table, (uint32_t)0x1000, (uint32_t)ptr_pa,
                    PAGE_PRESENT | PAGE_WRITE_ALLOW);
    int *ptr_va = (int *)(0x1000);
    *ptr_va = 198913;

    if (*ptr_pa != 198913) {
        println("paging map fail..");
    } else {
        println("paging map pass..");
        print_int(*ptr_pa);
    }
}