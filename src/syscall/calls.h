#ifndef CALLS_H
#define CALLS_H
#include "idt/idt.h"

enum SYS_CALLS {
    SYS_CALL0_HELLO_WORLD_SUM,
    SYS_CALL1_PRINT,
    SYS_CALL2_GET_CHAR,
    SYS_CALL3_PUT_CHAR,
    SYS_CALL4_MMAP,
    SYS_CALL5_MUNMAP,
    SYS_CALL6_CLEAR_SCREEN,
};

void *syscall_print(struct interrupt_frame *frame);
void *syscall_get_char(struct interrupt_frame *frame);
void *syscall_put_char(struct interrupt_frame *frame);
void *syscall_mmap(struct interrupt_frame *frame);
void *syscall_munmap(struct interrupt_frame *frame);
void *syscall_clear_screen(struct interrupt_frame *frame);

#endif