#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "task/process.h"
#include "task/task.h"

typedef int (*KEYBOARD_INIT_FUNC)();

struct keyboard {
    KEYBOARD_INIT_FUNC init;
    char name[20];
};

void keyboard_init();
void keyboard_push(char c, struct process *proc);
char keyboard_pop(struct task *task);

#endif