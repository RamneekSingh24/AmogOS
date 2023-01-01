#ifndef PROCESS_H
#define PROCESS_H

#include "config.h"
#include "task/task.h"
#include <stdint.h>

enum { PROC_UNUSED, PROC_BLOCKED, PROC_READY, PROC_RUNNING };

struct process {
    uint16_t pid;
    uint8_t status;

    struct task *task;

    void *code_data_paddr; // (code + data will be loaded contiguouly)
    void *stack_paddr;

    uint32_t size;

    void *chan;

    char program_file[FS_MAX_PATH_LEN + 10];

    void *vmem_blocks[PROCESS_VMEM_VMEM_MAX_BLOCK_COUNT];
    int open_files[PROCESS_MAX_OPEN_FILES];

    struct keyboard_buffer {
        char buf[PROCESS_KEYBOARD_BUFFER_SIZE];
        int head;
        int tail;
    } keyboard;
};

void procs_init();
int process_new(const char *filename, struct process **process_out);
void set_current_process(struct process *proc);
struct process *process_current();

#endif