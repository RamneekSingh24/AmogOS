#include "idt/idt.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"
#include "console/console.h"

static int _create_proccess(const char *file_path, int argc, char **argv) {
    // TODO: For now, we don't support arguments
    struct process *proc = 0;
    int res = process_new(file_path, &proc);
    if (res != STATUS_OK) {
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

// int create_proccess(const char* file_path, int argc, char** argv);
void *syscall_create_process(struct interrupt_frame *frame) {
    char **argv = (char **)task_get_stack_item(task_current(), 0);
    int argc = (int)task_get_stack_item(task_current(), 1);
    char *file_path = (char *)task_get_stack_item(task_current(), 2);
    return (void*) _create_proccess(file_path, argc, argv);
}