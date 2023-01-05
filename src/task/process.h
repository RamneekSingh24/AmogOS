#ifndef PROCESS_H
#define PROCESS_H

#include "config.h"
#include "task/task.h"
#include <stdint.h>

enum { PROC_FILE_TYPE_ELF, PROC_FILE_TYPE_BINARY };

enum { PROC_UNUSED, PROC_CREATING, PROC_CAN_START, PROC_ZOMBIE };

#define PROC_WAIT_NONE -1

struct process {
    uint16_t pid;
    uint16_t parent_pid;
    int exit_status;

    uint8_t status;

    // main thread of the process
    struct task *task;

    int file_type;

    union {
        void *code_data_paddr; // For binary files: (code + data will be loaded
                               // contiguously)
        struct elf_file *elf_file; // For elf files
    };

    // stack of the main thread
    // physical addr of bottom(lower), not top of the stack
    // stack actually starts at stack_paddr + DEFAULT_USER_STACK_SIZE
    void *stack_paddr;

    uint32_t size;

    void *chan;

    char program_file[FS_MAX_PATH_LEN + 10];

    void *vmem_blocks_start[PROCESS_VMEM_MAX_BLOCKS];
    void *vmem_blocks_end[PROCESS_VMEM_MAX_BLOCKS];
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
int process_exit(struct process *proc, int status);
int process_waitpid(struct process *proc, int waitpid);
int process_add_arguments(struct process *proc, int argc, int len, char *args);
int process_add_vmem_block(struct process *proc, void *va_start, void *va_end);
int process_get_vmem_block(struct process *proc, void *va_start);
int process_free_vmem_block(struct process *proc, void *va_start);
void process_set_parent_pid(struct process *proc, int pid);

#endif