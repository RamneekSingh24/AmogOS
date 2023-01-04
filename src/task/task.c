#include "task.h"
#include "config.h"
#include "console/console.h"
#include "invariants.h"
#include "kernel.h"
#include "loader/elfloader.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "process.h"
#include "status.h"

struct task *curr_task = 0;
struct task *tasks_ll_head = 0;
struct task *tasks_ll_tail = 0;

struct task *task_current() { return curr_task; }

int task_init(struct task *task, struct process *proc) {
    memset(task, 0, sizeof(struct task));
    // For now creating a full 4GB page table for each task
    // TODO: Everything is mapped into the task's kernel space for now.
    // Fix this when we have page fault handler so we only map pages for the
    // required memory.
    int res = paging_create_4gb_page_tables(PAGE_PRESENT | PAGE_WRITE_ALLOW,
                                            &task->page_table);
    if (res != STATUS_OK) {
        return res;
    }

    task->registers.eip = DEFAULT_USER_PROG_ENTRY;
    if (proc->file_type == PROC_FILE_TYPE_ELF) {
        task->registers.eip = elf_header(proc->elf_file)->e_entry;
    }

    task->registers.ss = DEFAULT_USER_DATA_SEGMENT;
    task->registers.esp = DEFAULT_USER_STACK_START;
    task->registers.cs = DEFAULT_USER_CODE_SEGMENT;

    task->proc = proc;
    task->state = TASK_READY;

    return STATUS_OK;
}

struct task *task_new(struct process *proc) {
    int res = 0;

    struct task *task = kzalloc(sizeof(struct task));

    if (!task) {
        return 0;
    }

    res = task_init(task, proc);
    if (res != STATUS_OK) {
        kfree(task);
        return 0;
    }

    if (!tasks_ll_head) {
        tasks_ll_head = task;
        tasks_ll_tail = task;
        task->prev = 0;
        task->next = 0;
    } else {
        tasks_ll_tail->next = task;
        task->prev = tasks_ll_tail;
        task->next = 0;
        tasks_ll_tail = task;
    }

    return task;
}

struct task *task_get_next() {
    if (!curr_task) {
        return 0;
    }
    if (!curr_task->next) {
        return tasks_ll_head;
    }
    return curr_task->next;
}

int task_free(struct task *task) {
    if (!task) {
        return 0;
    }
    if (task->state != TASK_DEAD) {
        panic("Trying to free a task that is not dead");
    }

    paging_free_page_table(&task->page_table);

    // remove from tasks ll
    if (task->prev) {
        task->prev->next = task->next;
    }
    if (task->next) {
        task->next->prev = task->prev;
    }
    if (task == tasks_ll_head) {
        tasks_ll_head = task->next;
    }
    if (task == tasks_ll_tail) {
        tasks_ll_tail = task->prev;
    }
    if (task == curr_task) {
        curr_task = 0;
    }

    kfree(task);
    return 0;
}

int task_switch(struct task *task) {
    if (curr_task && curr_task->state == TASK_RUNNING) {
        // set only if curr task exits and it is running
        // other wise the task may be modified by other parts of the code
        // or even be set to null
        // for ex during exit, task state is set to dead
        // during a blocking call, task state will be set to blocked
        curr_task->state = TASK_READY;
    }
    curr_task = task;
    task->state = TASK_RUNNING;
    set_current_process(task->proc);
    // DESIGN INVARIANT: All the kernel memory is mapped(identity) into the task
    // page table So this is SAFE
    paging_switch(&task->page_table);
    return 0;
}

int task_switch_and_run(struct task *task) {
    if (task->state != TASK_READY) {
        panic("Can't switch to a non ready task");
    }
    task_switch(task);
    task_return(&task->registers);
    return 0;
}

void task_switch_and_run_any() {
    assert_single_cpu();
    assert_interrupt_handler_cli_always();

    struct task *task = tasks_ll_head;
    while (task) {
        if (task->state == TASK_READY) {
            task_switch_and_run(task);
        }
        task = task->next;
    }
    // if single cpu and cli then we are in a deadlock!
    panic("Deadlock: No ready task found\n");
}

// Run the init process, and sets the parent pid to itself
void task_run_init_task() {
    if (!tasks_ll_head) {
        panic("Can't start init task: no curr tasks exists\n");
    }
    tasks_ll_head->proc->parent_pid = tasks_ll_head->proc->pid;
    task_switch_and_run(tasks_ll_head);
}

void task_save_current_state(struct interrupt_frame *frame) {
    if (!curr_task) {
        panic("Saved state called but no curr task");
    }

    curr_task->registers.eip = frame->eip;
    curr_task->registers.cs = frame->cs;
    curr_task->registers.eflags = frame->eflags;
    curr_task->registers.esp = frame->esp;
    curr_task->registers.ss = frame->ss;
    curr_task->registers.eax = frame->eax;
    curr_task->registers.ebp = frame->ebp;
    curr_task->registers.ebx = frame->ebx;
    curr_task->registers.ecx = frame->ecx;
    curr_task->registers.edi = frame->edi;
    curr_task->registers.edx = frame->edx;
    curr_task->registers.esi = frame->esi;
}

int copy_data_from_user(void *dst, void *src, uint32_t nbytes) {
    // check if src is in user space
    // user is allowed to user [KHEAP_SAFE_BOUNDARY, ... END_OF_MEM)
    if (((uint32_t)src) < KHEAP_SAFE_BOUNDARY ||
        ((uint32_t)src) + nbytes < KHEAP_SAFE_BOUNDARY) {
        panic("Invalid user memory access");
        return -STATUS_INVALID_USER_MEM_ACCESS;
    }
    memcpy(dst, src, nbytes);

    return STATUS_OK;
};

// checks if ptr is in user space
int verify_user_pointer(void *ptr) {
    if (((uint32_t)ptr) < KHEAP_SAFE_BOUNDARY) {
        return -STATUS_INVALID_USER_MEM_ACCESS;
    }
    return STATUS_OK;
}

// ASSUME: all items are 4 bytes
void *task_get_stack_item(struct task *task, int index) {
    uint32_t *stack = (uint32_t *)task->registers.esp;
    uint32_t item;
    int res = copy_data_from_user(&item, &stack[index], sizeof(uint32_t));
    if (res != STATUS_OK) {
        return 0;
    }
    return (void *)item;
}

// NOT USED
// int task_page() {
//     user_registers();
//     task_switch(curr_task);
//     return 0;
// }
