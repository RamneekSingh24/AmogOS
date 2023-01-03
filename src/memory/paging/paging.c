#include "paging.h"
#include "config.h"
#include "console/console.h"
#include "invariants.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "status.h"

struct page_table_32b kpage_table;

struct page_table_32b *current_pt[N_CPU_MAX];

extern void paging_load_dir(uint32_t *cr3);
extern void paging_enable();

// Creates page tables for the complete 4gb 32 bit address space with identity
// mapping.
int paging_create_4gb_page_tables(uint8_t flags,
                                  struct page_table_32b *page_table) {
    page_table_entry *pt_dir =
        kzalloc(sizeof(page_table_entry) * NUM_PAGE_TABLE_ENTRIES);
    if (!pt_dir) {
        return -STATUS_NOT_ENOUGH_MEM;
    }
    int offset = 0;
    for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
        page_table_entry *pt_i =
            kzalloc(sizeof(page_table_entry) * NUM_PAGE_TABLE_ENTRIES);
        if (!pt_i) {
            if (pt_dir) {
                kfree(pt_dir);
            }
            for (int j = 0; j < i; j++) {
                if (pt_dir) {
                    uint32_t pte = pt_dir[j];
                    uint32_t *pt_j = (uint32_t *)(pte & PAGE_FRAME_LOC_MASK);
                    kfree(pt_j);
                }
            }
            return -STATUS_NOT_ENOUGH_MEM;
        }

        for (int j = 0; j < NUM_PAGE_TABLE_ENTRIES; j++) {
            pt_i[j] = (offset + j * PAGE_SIZE) | flags;
        }

        offset += PAGE_SIZE * NUM_PAGE_TABLE_ENTRIES;
        uint32_t pt_i_frame_num = (uint32_t)(pt_i) / PAGE_SIZE;
        pt_dir[i] = (pt_i_frame_num * PAGE_SIZE) | flags | PAGE_WRITE_ALLOW;
    }

    page_table->cr3 = pt_dir;
    page_table->num_levels = 2;
    return STATUS_OK;
}

// Creates ONLY the 1st level page directory(zeroed out)
int paging_init_new_mapping(struct page_table_32b *pt) {
    pt->num_levels = 2;
    page_table_entry *pt_dir =
        kzalloc(sizeof(page_table_entry) * NUM_PAGE_TABLE_ENTRIES);
    if (!pt_dir) {
        return -STATUS_NOT_ENOUGH_MEM;
    }
    pt->cr3 = pt_dir;
    return STATUS_OK;
}

// Creates a new vpn -> pfn mapping
// Allocates the page table if was not present
// Returns NOT_ENOUGH_MEM if can't alloc new pt
// or INVALID_ARG if vpn was already present and overwrite=false
int paging_new_vpn_to_pfn(struct page_table_32b *pt, uint32_t vpn, uint32_t pfn,
                          uint8_t flags, bool overwrite) {
    int dir_idx = vpn >> 10;
    page_table_entry *second_level_pt;
    uint32_t pte = pt->cr3[dir_idx];

    if ((pte & PAGE_PRESENT) == 0) {
        second_level_pt =
            kzalloc(sizeof(page_table_entry) * NUM_PAGE_TABLE_ENTRIES);
        if (!second_level_pt) {
            return -STATUS_NOT_ENOUGH_MEM;
        }
    } else {
        second_level_pt = (page_table_entry *)(pte & PAGE_FRAME_LOC_MASK);
    }

    int second_level_pt_idx = vpn % (1 << 10);
    uint32_t second_level_pte = second_level_pt[second_level_pt_idx];

    if (second_level_pte & PAGE_PRESENT) {
        // already present
        if (overwrite) {
            // free up old page: DESIGN INVARIANT: No Page Sharing
            void *page_addr = (void *)(pte & PAGE_FRAME_LOC_MASK);
            // TODO: FIXME: Do not free the page
            // since kernel pages are shared by all
            // which we do not want to free.
            // wait until shared pages are introduced
            // and only then free pages
            if (page_addr) {
                // kfree(page_addr);
            }
        } else {
            return -STATUS_INVALID_ARG;
        }
    }

    uint32_t page_addr = pfn * PAGE_SIZE;
    second_level_pt[second_level_pt_idx] = page_addr | flags;

    // overwrite flags in dir (the hardware uses `and` of the permissions in the
    // 2 levels)
    if (overwrite) {
        pt->cr3[dir_idx] = (uint32_t)second_level_pt | flags;
    }

    return STATUS_OK;
}

// TODO: add more tlb flush functions

// Flushes the tlb for the given virtual address
static inline void __native_flush_tlb_single(unsigned int addr) {
    asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

// Free the frame mapping to the vpn
// Zeroes out the pte
// DESIGN INVARIANT: No Shared Pages
int paging_free_vpn(struct page_table_32b *pt, uint32_t vpn) {
    assert_no_page_sharing();
    int dir_idx = vpn >> 10;
    uint32_t pte = pt->cr3[dir_idx];
    if ((pte & PAGE_PRESENT) == 0) {
        return 0;
    }
    page_table_entry *second_level_pt =
        (page_table_entry *)(pte & PAGE_FRAME_LOC_MASK);

    int second_level_pt_idx = vpn % (1 << 10);
    uint32_t second_level_pte = second_level_pt[second_level_pt_idx];
    if ((second_level_pte & PAGE_PRESENT) == 0) {
        return 0;
    }
    void *paddr = (void *)(second_level_pte & PAGE_FRAME_LOC_MASK);
    if (paddr) {
        // NO FREEING FOR NOW
        // kfree(paddr); // DESIGN INVARIANT: NO SHARED PAGES
    }

    second_level_pt[second_level_pt_idx] = 0x00;

    // Very important: flush the tlb
    assert_single_cpu(); // Doesn't work on SMP
    __native_flush_tlb_single(vpn * PAGE_SIZE);

    return 0;
}

// Requires vaddr_start and vaddr_end to be page aligned
// Allocates new frames and creates mapping for the
// virtual addr space [vaddr_start... vaddr_end)
int paging_alloc_mapping(struct page_table_32b *pt, uint32_t vaddr_start,
                         uint32_t vaddr_end, uint8_t flags) {
    if (vaddr_start % PAGE_SIZE != 0 || vaddr_end % PAGE_SIZE != 0) {
        return -STATUS_INVALID_ARG;
    }

    if (vaddr_end <= vaddr_start) {
        return -STATUS_INVALID_ARG;
    }

    uint32_t start_vpn = vaddr_start / PAGE_SIZE;
    uint32_t end_vpn = vaddr_end / PAGE_SIZE;
    if (vaddr_end % PAGE_SIZE != 0) {
        end_vpn++;
    }

    for (int i = start_vpn; i < end_vpn; i++) {
        uint32_t new_paddr = (uint32_t)kzalloc(PAGE_SIZE);
        if (new_paddr == 0) {
            for (int j = start_vpn; j < i; j++) {
                paging_free_vpn(pt, j);
            }
            return -STATUS_NOT_ENOUGH_MEM;
        }
        uint32_t pfn = new_paddr / PAGE_SIZE;
        int res = paging_new_vpn_to_pfn(pt, i, pfn, flags, true);
        if (res < 0) {
            for (int j = start_vpn; j < i; j++) {
                paging_free_vpn(pt, j);
            }
            // NO FREEING FOR NOW
            // kfree((uint32_t *)new_paddr);
            return res;
        }
    }

    return STATUS_OK;
}

// Frees all the page tables and page dir
int paging_free_page_table(struct page_table_32b *pt) {
    if (pt->num_levels == 1) {
        panic("single level pages not support yet..!");
    }
    for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
        uint32_t pte = pt->cr3[i];
        uint32_t *second_level_pt = (uint32_t *)(pte & PAGE_FRAME_LOC_MASK);
        kfree(second_level_pt);
    }
    kfree(pt->cr3);
    return 0;
}

// Frees up a virtual addr space [start...end)
// inputs should be page aligned
// ** DOES NOT FREE UP PAGE TABLES **
// User should call paging_free_page_table(..)
// This should work even if the physical memory is not contiguous
int paging_free_va(struct page_table_32b *pt, uint32_t vaddr_start,
                   uint32_t vaddr_end) {
    if (vaddr_end < vaddr_start) {
        return -STATUS_INVALID_ARG;
    }

    uint32_t start_vpn = vaddr_start / PAGE_SIZE;
    uint32_t end_vpn = vaddr_end / PAGE_SIZE;
    if (vaddr_end % PAGE_SIZE != 0) {
        end_vpn++;
    }
    for (uint32_t i = start_vpn; i < end_vpn; i++) {
        paging_free_vpn(pt, i);
    }
    return STATUS_OK;
}

// DESIGN INVARIANT: No Shared Pages
// static int paging_free_frames_and_pt_in_dir(page_table_entry *pt_dir, int
// idx) {
//     // TODO
//     return 0;
// }

// DESIGN INVARIANT: No Shared Pages
int paging_free_table_and_frames(struct page_table_32b *pt) {
    // TOOD:
    return 0;
}

void paging_switch(struct page_table_32b *pt) {
    // int cpu = get_cpu_id();
    int cpu = 0;

    paging_load_dir(pt->cr3);

    current_pt[cpu] = pt;
}

void paging_load_kernel_page_table() { paging_switch(&kpage_table); }

void kpaging_init() {
    // Creates page tables for the complete 4gb 32 bit add
    int res = paging_create_4gb_page_tables(PAGE_PRESENT | PAGE_WRITE_ALLOW,
                                            &kpage_table);
    if (res != STATUS_OK) {
        panic("Failed to create kernel page table");
    }
    paging_switch(&kpage_table);
    paging_enable();
}

bool is_page_aligned(uint32_t addr) { return (addr & (PAGE_SIZE - 1)) == 0; }

// creates a mapping for vaddr -> paddr (should be page aligned)
// ** ASSUMES THE 2nd level PAGE_TABLE and DIR is alloced **
// ** OVERWRITES old page without checking and freeing existing mapped frame **
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

    uint32_t *pt_i = (uint32_t *)(pt_dir[pt_dir_index] & PAGE_FRAME_LOC_MASK);
    pt_i[pt_index] = phys_addr | flags;

    return 0;
}

// aligns addr to previous(lower) page boundary
void *paging_down_align_addr(void *addr) {
    if ((uint32_t)addr % PAGE_SIZE != 0) {
        addr -= ((uint32_t)addr % PAGE_SIZE);
    }
    return addr;
}

// aligns addr to next(higher) page boundary
void *paging_up_align_addr(void *addr) {
    if ((uint32_t)addr % PAGE_SIZE != 0) {
        addr += (PAGE_SIZE - ((uint32_t)addr % PAGE_SIZE));
    }
    return addr;
}

// ** Memory leak : in case of failure, does not free up any memory for page
// tables if alloced
// ** User should call paging_free_page_table(..) to free up page tables
// Maps [data_va_start... data_va_end) to [data_paddr :)
// ** Overwrites old last level mappings
// ** Overwrites old dir level flags
int paging_map_memory_region(struct page_table_32b *pt, void *data_va_start,
                             void *data_paddr, void *data_va_end,
                             uint8_t flags) {

    if (!is_page_aligned((uint32_t)data_va_start) ||
        !is_page_aligned((uint32_t)data_paddr) ||
        !is_page_aligned((uint32_t)data_va_end)) {
        return -STATUS_INVALID_ARG;
    }

    uint32_t vpn_start = (uint32_t)(data_va_start) / PAGE_SIZE;
    uint32_t vpn_end = (uint32_t)(data_va_end) / PAGE_SIZE;
    uint32_t pfn_start = (uint32_t)(data_paddr) / PAGE_SIZE;

    if (vpn_end <= vpn_start) {
        return -STATUS_INVALID_ARG;
    }

    for (int i = 0; i + vpn_start < vpn_end; i++) {
        int res = paging_new_vpn_to_pfn(pt, vpn_start + i, pfn_start + i, flags,
                                        true);
        if (res != STATUS_OK) {
            return res;
        }
    }

    return STATUS_OK;
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