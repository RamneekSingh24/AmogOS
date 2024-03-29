#include "keyboard.h"
#include "dev/ps2.h"
#include "task/process.h"

static struct keyboard *keyboard;

void keyboard_init() {
    keyboard = get_ps2_keyboard();
    keyboard->init();
}

void keyboard_push(char c, struct process *proc) {
    if (!proc) {
        return;
    }
    if (c == 0x00) {
        return;
    }
    proc->keyboard.buf[proc->keyboard.tail++] = c;
    proc->keyboard.tail %= PROCESS_KEYBOARD_BUFFER_SIZE;
}

char keyboard_pop(struct task *task) {
    struct process *p = task->proc;
    if (!p) {
        return 0x00;
    }
    char c = p->keyboard.buf[p->keyboard.head];
    if (c == 0x00) {
        return 0x00;
    }
    p->keyboard.head++;
    p->keyboard.head %= PROCESS_KEYBOARD_BUFFER_SIZE;
    return c;
}

void keyboard_set_capslock(struct keyboard *keyboard, int state) {
    keyboard->caps_lock_state = state;
}

void keyboard_toggle_capslock(struct keyboard *keyboard) {
    keyboard->caps_lock_state =
        (keyboard->caps_lock_state == KEYBOARD_STATE_CAPS_ON)
            ? KEYBOARD_STATE_CAPS_OFF
            : KEYBOARD_STATE_CAPS_ON;
}

int keyboard_get_capslock(struct keyboard *keyboard) {
    return keyboard->caps_lock_state;
}