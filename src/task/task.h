
#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "idt/idt.h"
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

struct process;

struct task {
    struct page_table_32b page_table;

    struct registers registers;

    struct process *proc;

    struct task *next;

    struct task *prev;
};

struct task *task_new(struct process *proc);
struct task *task_current();
struct task *task_get_next();
int task_free(struct task *task);

int task_switch(struct task *task);
int task_page();
void task_run_init_task();

// asm functions
void task_return(struct registers *regs);
void restore_general_purpose_registers(struct registers *regs);
void user_registers();
void task_save_current_state(struct interrupt_frame *frame);

#endif