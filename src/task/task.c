#include "task.h"
#include "config.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

struct task *curr_task = 0;
struct task *tasks_ll_head = 0;
struct task *tasks_ll_tail = 0;

struct task *task_current() {
    return curr_task;
}

int task_init(struct task *task) {
    memset(task, 0, sizeof(struct task));
    int res = paging_create_va(PAGE_PRESENT | PAGE_USER_ACCESS_ALLOW,
                               &task->page_table);
    if (res != STATUS_OK) {
        return res;
    }

    task->registers.eip = DEFAULT_USER_PROG_ENTRY;
    task->registers.ss = DEFAULT_USER_DATA_SEGMENT;
    task->registers.esp = DEFAULT_USER_STACK_START;

    return STATUS_OK;
}

struct task *task_new() {
    int res = 0;

    struct task *task = kzalloc(sizeof(struct task));

    if (!task) {
        return 0;
    }

    res = task_init(task);
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