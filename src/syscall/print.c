#include "console/console.h"
#include "idt/idt.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "task/task.h"

// print(void* str, uint32_t len);
void *syscall_print(struct interrupt_frame *frame) {
    void *str = task_get_stack_item(task_current(), 1);
    uint32_t len = (uint32_t)task_get_stack_item(task_current(), 0);
    char *buf = kzalloc(len + 1);
    if (!buf) {
        return 0;
    }
    copy_data_from_user(buf, str, len);
    // memcpy(buf, str, len);
    printn(buf, len);
    println("");
    kfree(buf);
    return 0;
}