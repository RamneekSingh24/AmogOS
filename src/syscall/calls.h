#ifndef CALLS_H
#define CALLS_H
#include "idt/idt.h"

void *syscall_print(struct interrupt_frame *frame);

#endif