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

static int add_vmem_block(struct process *proc, void *va_start, void *va_end) {
    for (int i = 0; i < PROCESS_VMEM_MAX_BLOCKS; i++) {
        if (proc->vmem_blocks_start[i] == 0) {
            proc->vmem_blocks_start[i] = va_start;
            proc->vmem_blocks_end[i] = va_end;
            return i;
        }
    }
    return -STATUS_OUT_OF_VMEM_BLOCKS;
}

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
    int block_idx =
        add_vmem_block(task_current()->proc, va_start_aligned, va_end_aligned);

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

    struct page_table_32b *pt = &task_current()->page_table;

    // find the memory region
    int block_idx = -1;
    for (int i = 0; i < PROCESS_VMEM_MAX_BLOCKS; i++) {
        if (task_current()->proc->vmem_blocks_start[i] == addr) {
            block_idx = i;
            break;
        }
    }

    if (block_idx < 0) {
        return (void *)-STATUS_INVALID_MEMORY_REGION;
    }

    void *va_start = task_current()->proc->vmem_blocks_start[block_idx];
    void *va_end = task_current()->proc->vmem_blocks_end[block_idx];

    // unmap the memory region
    int res = paging_free_va(pt, (uint32_t)va_start, (uint32_t)va_end);
    if (res != STATUS_OK) {
        return (void *)res;
    }

    // remove from task's memory regions
    task_current()->proc->vmem_blocks_start[block_idx] = 0;
    task_current()->proc->vmem_blocks_end[block_idx] = 0;

    return STATUS_OK;
}