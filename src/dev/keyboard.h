#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "task/process.h"
#include "task/task.h"

#define KEYBOARD_STATE_CAPS_ON 1
#define KEYBOARD_STATE_CAPS_OFF 0

typedef int (*KEYBOARD_INIT_FUNC)();

struct keyboard {
    KEYBOARD_INIT_FUNC init;
    char name[20];
    int caps_lock_state;
};

void keyboard_init();
void keyboard_push(char c, struct process *proc);
char keyboard_pop(struct task *task);
void keyboard_set_capslock(struct keyboard *keyboard, int state);
void keyboard_toggle_capslock(struct keyboard *keyboard);
int keyboard_get_capslock(struct keyboard *keyboard);

#endif