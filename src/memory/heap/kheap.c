#include "kheap.h"
#include "console/console.h"
#include "memory/memory.h"
#include "status.h"

#include <stdbool.h>

static bool validate_ptr_alignment(void *ptr) {
    return ((unsigned int)ptr % KHEAP_BLOCK_SIZE) == 0;
}

static bool validate_kheap_table(void *kheap_start, void *kheap_end,
                                 struct KHEAP_ENTRY_TABLE *entry_table) {

    size_t table_size = (size_t)(kheap_end - kheap_start);
    size_t total_blocks = table_size / KHEAP_BLOCK_SIZE;
    if (entry_table->num_entries != total_blocks) {
        return false;
    }

    return true;
}

static int kheap_create(struct KHEAP *kheap, void *kheap_physical_start_addr,
                        void *kheap_physical_end_addr,
                        struct KHEAP_ENTRY_TABLE *entry_table) {

    if (!validate_ptr_alignment(kheap_physical_start_addr) ||
        !validate_ptr_alignment(kheap_physical_end_addr)) {
        return -STATUS_INVALID_ARG;
    }

    memset(kheap, 0, sizeof(struct KHEAP));

    kheap->kheap_physical_start_addr = kheap_physical_start_addr;
    kheap->entry_table = entry_table;

    if (!validate_kheap_table(kheap_physical_start_addr,
                              kheap_physical_end_addr, entry_table)) {
        return -STATUS_INVALID_ARG;
    }

    size_t kheap_table_size =
        sizeof(KHEAP_BLOCK_TABLE_ENTRY) * entry_table->num_entries;

    memset(entry_table->entries, KHEAP_BLOCK_TABLE_ENTRY_FREE,
           kheap_table_size);

    return STATUS_OK;
}

static struct KHEAP kheap;
static struct KHEAP_ENTRY_TABLE kheap_entry_table;

int kheap_init() {
    size_t total_table_entries = KERNEL_HEAP_SIZE_BYTES / KHEAP_BLOCK_SIZE;
    kheap_entry_table.entries = (KHEAP_BLOCK_TABLE_ENTRY *)KHEAP_TABLE_ADDR;

    kheap_entry_table.num_entries = total_table_entries;

    void *kheap_end = (void *)((size_t)KHEAP_ADDR + KERNEL_HEAP_SIZE_BYTES);

    int res =
        kheap_create(&kheap, (void *)KHEAP_ADDR, kheap_end, &kheap_entry_table);

    if (res < 0) {
        print("error while creating kheap\n");
        return -STATUS_GENERIC_ERROR;
    }

    // kheap.entry_table = &kheap_entry_table;
    // kheap.kheap_physical_start_addr = (void*)KHEAP_ADDR;

    return STATUS_OK;
}

static int get_kheap_entry_type(KHEAP_BLOCK_TABLE_ENTRY entry) {
    return entry & 0x0f;
}

static void *kheap_block_to_addr(int block_index) {
    return (void *)((size_t)kheap.kheap_physical_start_addr +
                    (block_index * KHEAP_BLOCK_SIZE));
}

static size_t kheap_addr_to_block_index(void *addr) {
    return ((size_t)addr - (size_t)kheap.kheap_physical_start_addr) /
           KHEAP_BLOCK_SIZE;
}

void *kmalloc(size_t size) {

    // int search_start = 0;

    size_t num_blocks = (size + KHEAP_BLOCK_SIZE - 1) / (KHEAP_BLOCK_SIZE);

    size_t first_alloc_block;
    bool found = false;

    // search for contiguous blocks that fit our request
    for (int i = 0; i < kheap.entry_table->num_entries; i++) {
        if (get_kheap_entry_type(kheap.entry_table->entries[i]) ==
            KHEAP_BLOCK_TABLE_ENTRY_FREE) {
            int j = i;
            for (; j < i + num_blocks; j++) {
                if (get_kheap_entry_type(kheap.entry_table->entries[j]) !=
                    KHEAP_BLOCK_TABLE_ENTRY_FREE) {
                    break;
                }
            }

            if (j == i + num_blocks) {
                first_alloc_block = i;
                found = true;
                break; // exit j's loop
            }

            if (found) {
                break; // exit i's loop
            }
        }
    }

    if (!found) {
        print("kheap: out of memory\n");
        return NULL;
    }

    // mark the blocks as allocated

    int start_block = first_alloc_block;
    int end_block = first_alloc_block + num_blocks - 1;

    KHEAP_BLOCK_TABLE_ENTRY entry =
        KHEAP_BLOCK_TABLE_ENTRY_TAKEN | KHEAP_BLOCK_IS_FIRST;

    if (num_blocks > 1) {
        entry |= KHEAP_BLOCK_HAS_NEXT;
    }

    kheap.entry_table->entries[start_block] = entry;

    for (int i = start_block + 1; i <= end_block; i++) {
        entry = KHEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i < end_block) {
            entry |= KHEAP_BLOCK_HAS_NEXT;
        }
        kheap.entry_table->entries[i] = entry;
    }

    // print("used start indx..");
    // print_int((int) start_block);
    // print("\n");

    // search_start = first_alloc_block + num_blocks; // next time we start
    // searching from here
    return kheap_block_to_addr(first_alloc_block);
}

int kfree(void *ptr) {

    size_t start_block = kheap_addr_to_block_index(ptr);

    for (size_t i = start_block; i < kheap.entry_table->num_entries; i++) {
        KHEAP_BLOCK_TABLE_ENTRY entry = kheap.entry_table->entries[i];
        kheap.entry_table->entries[i] = KHEAP_BLOCK_TABLE_ENTRY_FREE;
        // print("freed indx..");
        // print_int((int) i);
        // print("\n");
        if (!(entry & KHEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }

    return 0;
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    memset(ptr, 0x0, size);
    return ptr;
}

int kheap_num_free_blocks() {
    int num_free = 0;
    for (int i = 0; i < kheap.entry_table->num_entries; i++) {
        if (get_kheap_entry_type(kheap.entry_table->entries[i]) ==
            KHEAP_BLOCK_TABLE_ENTRY_FREE) {
            num_free++;
        }
    }
    return num_free;
}

// ----------------------- tests ------------------- //

void kheap_test() {
    void *ptr = kmalloc(50);
    void *ptr2 = kmalloc(5000);
    void *ptr3 = kmalloc(5300);
    kfree(ptr2);
    void *ptr4 = kmalloc(15000);

    if (ptr || ptr2 || ptr3 || ptr4) {
    };
}