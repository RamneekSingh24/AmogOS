#include "syscall.h"
#include "idt/idt.h"
#include "console/console.h"



void* syscall_hello_sum(struct interrupt_frame *frame) {
    print("Hello world syscall! Sum: ");
    frame->ecx = 1989;
    return 0;
}


void register_syscalls() {
     syscall_register_command(SYS_CALL0_HELLO_SUM, syscall_hello_sum);
}


