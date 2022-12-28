
#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"

struct registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};

struct task {
    struct page_table_32b page_table;

    struct registers registers;

    struct task *next;

    struct task *prev;
};

struct task *task_new();
struct task *task_current();
struct task *task_get_next();
int task_free(struct task *task);

#endif