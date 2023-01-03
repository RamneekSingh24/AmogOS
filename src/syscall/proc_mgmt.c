#include "console/console.h"
#include "idt/idt.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"

// args should point to start of the arguments, which are joined by the null
// terminator i.e. args = location("\0".join(arguments)) len should be the
// length of the args string (including all the null terminator) Example:
// argc=3, len = **12**, args = location("abc\0def\0ghi\0") int
// create_proccess(const char* file_path, int argc, int len, char* args);
static int _create_proccess(const char *file_path, int argc, int len,
                            char *args) {
    // TODO: For now, we don't support arguments
    struct process *proc = 0;
    int res = process_new(file_path, &proc);
    if (res != STATUS_OK) {
        return res;
    }
    res = process_add_arguments(proc, argc, len, args);
    if (res != STATUS_OK) {
        // TODO: free the process if the arguments are not added
        return res;
    }
    // Artificially set the eax register to STATUS_OK
    // So that when the calling task is run again it will return STATUS_OK
    // This function though, will never return after task_switch_and_run
    task_current()->registers.eax = STATUS_OK;
    task_switch_and_run(proc->task);
    // We never reach here
    return proc->pid;
}

// int create_proccess(const char* file_path, int argc, int len, char* args);
void *syscall_create_process(struct interrupt_frame *frame) {
    char *args = (char *)task_get_stack_item(task_current(), 0);
    int len = (int)task_get_stack_item(task_current(), 1);
    int argc = (int)task_get_stack_item(task_current(), 2);
    char *file_path = (char *)task_get_stack_item(task_current(), 3);
    return (void *)_create_proccess(file_path, argc, len, args);
}