#include "task.h"
#include "config.h"
#include "kernel.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "process.h"
#include "status.h"

struct task *curr_task = 0;
struct task *tasks_ll_head = 0;
struct task *tasks_ll_tail = 0;

struct task *task_current() {
    return curr_task;
}

int task_init(struct task *task, struct process *proc) {
    memset(task, 0, sizeof(struct task));
    // For now creating a full 4GB page table for each task
    // TODO: Everything is accessable from user space for now.
    // Fix this when we have page fault handler
    int res = paging_create_4gb_page_tables(PAGE_PRESENT | PAGE_WRITE_ALLOW |
                                                PAGE_USER_ACCESS_ALLOW,
                                            &task->page_table);
    if (res != STATUS_OK) {
        return res;
    }

    task->registers.eip = DEFAULT_USER_PROG_ENTRY;
    task->registers.ss = DEFAULT_USER_DATA_SEGMENT;
    task->registers.esp = DEFAULT_USER_STACK_START;
    task->registers.cs = DEFAULT_USER_CODE_SEGMENT;

    task->proc = proc;

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
        curr_task = task;
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
        curr_task = task_get_next();
    }

    kfree(task);
    return 0;
}

int task_switch(struct task *task) {
    curr_task = task;
    // DESIGN INVARIANT: All the kernel memory is mapped(identity) into the task
    // page table So this is SAFE
    paging_switch(&task->page_table);
    return 0;
}

int task_page() {
    user_registers();
    task_switch(curr_task);
    return 0;
}

void task_run_init_task() {
    if (!curr_task) {
        panic("Can't start init task: no curr tasks exists\n");
    }
    task_switch(tasks_ll_head);
    task_return(&tasks_ll_head->registers);
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