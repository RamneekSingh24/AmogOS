#include "syscall.h"
#include "calls.h"
#include "console/console.h"
#include "idt/idt.h"
#include "task/task.h"

void *syscall_hello_sum(struct interrupt_frame *frame) {
    // print("Hello world syscall! Sum: ");
    frame->ecx = 1989;
    int v2 = (int)task_get_stack_item(task_current(), 1);
    int v1 = (int)task_get_stack_item(task_current(), 0);
    print_int(v1 + v2);
    println("");
    return (void *)(v1 + v2);
}

void register_syscalls() {
    syscall_register_command(SYS_CALL1_PRINT, syscall_print);
    // syscall_register_command(SYS_CALL0_HELLO_SUM, syscall_hello_sum);
}
