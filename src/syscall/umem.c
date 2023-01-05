#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"

enum {
    O_READ = 1,
    O_WRITE = 2,
    O_EXEC = 4,
};

// int mmap(void* va_start, void* va_end, int flags);
// va_start and va_end must be page aligned
void *syscall_mmap(struct interrupt_frame *frame) {
    void *va_start = task_get_stack_item(task_current(), 2);
    void *va_end = task_get_stack_item(task_current(), 1);
    int user_flags = (uint32_t)task_get_stack_item(task_current(), 0);

    uint8_t page_flags = PAGE_PRESENT | PAGE_USER_ACCESS_ALLOW;
    if (user_flags & O_WRITE) {
        page_flags |= PAGE_WRITE_ALLOW;
    }

    // if (user_flags & O_EXEC) {
    //     page_flags |= PAGE_EXEC_ALLOW;
    // }

    void *va_start_aligned = paging_down_align_addr(va_start);
    void *va_end_aligned = paging_up_align_addr(va_end);
    if (va_start != va_start_aligned || va_end != va_end_aligned) {
        return (void *)-STATUS_INVALID_ARG;
    }

    // check if va_start_aligned and va_end_aligned are valid
    if (verify_user_pointer(va_start_aligned) != STATUS_OK ||
        verify_user_pointer(va_end_aligned) != STATUS_OK ||
        va_start_aligned >= va_end_aligned) {
        return (void *)-STATUS_INVALID_USER_MEM_ACCESS;
    }

    uint32_t nbytes = (uint32_t)va_end_aligned - (uint32_t)va_start_aligned;
    // alloc memory
    void *pa_start = kzalloc(nbytes);
    if (!pa_start) {
        return (void *)-STATUS_NOT_ENOUGH_MEM;
    }

    struct page_table_32b *pt = &task_current()->page_table;

    // add to task's memory regions
    int block_idx = process_add_vmem_block(task_current()->proc,
                                           va_start_aligned, va_end_aligned);

    if (block_idx < 0) {
        kfree(pa_start);
        return (void *)block_idx;
    }

    int res = paging_map_memory_region(pt, va_start_aligned, pa_start,
                                       va_end_aligned, page_flags);
    if (res != STATUS_OK) {
        task_current()->proc->vmem_blocks_start[block_idx] = 0;
        task_current()->proc->vmem_blocks_end[block_idx] = 0;
        kfree(pa_start);
        return (void *)res;
    }

    // everything ok here
    return (void *)STATUS_OK;
}

// Should only pass addr earlier passed as va_start to mmap
// unmaps the complete memory region returned by mmap
// int munmap(void* addr);
// returns 0 on success -STATUS_INVALID_MEMORY_REGION if invalid addr
void *syscall_munmap(struct interrupt_frame *frame) {
    void *addr = task_get_stack_item(task_current(), 0);

    return (void *)process_free_vmem_block(task_current()->proc, addr);
}