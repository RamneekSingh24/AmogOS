#ifndef CALLS_H
#define CALLS_H
#include "idt/idt.h"

void *syscall_print(struct interrupt_frame *frame);
void *syscall_get_char(struct interrupt_frame *frame);
void *syscall_put_char(struct interrupt_frame *frame);
void *syscall_mmap(struct interrupt_frame *frame);
void *syscall_munmap(struct interrupt_frame *frame);

#endif