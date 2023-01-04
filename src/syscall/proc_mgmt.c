#include "console/console.h"
#include "idt/idt.h"
#include "invariants.h"
#include "memory/heap/kheap.h"
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
    process_set_parent_pid(proc, task_current()->proc->pid);

    char *args_kspace = kzalloc(len);
    copy_data_from_user(args_kspace, args, len);

    res = process_add_arguments(proc, argc, len, args_kspace);
    kfree(args_kspace);
    if (res != STATUS_OK) {
        // TODO: free the process if the arguments are not added
        return res;
    }
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

void *syscall_exit(struct interrupt_frame *frame) {
    int status = (int)task_get_stack_item(task_current(), 0);
    assert_single_task_per_process();
    process_exit(task_current()->proc, status);

    task_switch_and_run_any();

    // We never return here
    return (void *)0;
}

void *syscall_wait_pid(struct interrupt_frame *frame) {
    int pid = (int)task_get_stack_item(task_current(), 0);
    return (void *)process_waitpid(task_current()->proc, pid);
}