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
