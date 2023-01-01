#include "idt.h"
#include "config.h"
#include "console/console.h"
#include "io/io.h"
#include "kernel.h"
#include "memory/memory.h"
#include "task/task.h"

extern void idt_load(struct idt_ptr *ptr);
extern void int0h();
extern void int21h();
extern void int80h();
extern void int_generic_h();
extern void enable_interrupts();
extern void disable_interrupts();

struct idt_entry idt[NUM_INTERRUPTS];
struct idt_ptr idtp;

int cli_count;

void pop_cli() {
    if (cli_count <= 0) {
        println("error pop cli with count already <= 0");
    } else {
        cli_count--;
        if (cli_count == 0)
            disable_interrupts();
    }
}

void push_cli() {
    if (cli_count < 0) {
        println("error push cli with count < 0");
    } else {
        cli_count++;
        if (cli_count == 1)
            enable_interrupts();
    }
}

void intr_0_handler() { print("Divide by zero exception\n"); }

void intr_21_handler() {
    print("Keyboard pressed\n");
    port_io_out_byte(MASTER_PIC_PORT, MASTER_PIC_INTR_ACK);
}

static SYSCALL_HANDLER sys_calls[NUM_SYS_CALLS];

void *syscall_handle_command(int command, struct interrupt_frame *frame) {
    if (command < 0 || command >= NUM_SYS_CALLS) {
        return 0;
    }

    SYSCALL_HANDLER sys_call = sys_calls[command];
    if (!sys_call) {
        return 0;
    }

    return sys_call(frame);
}

void syscall_register_command(int command_num, SYSCALL_HANDLER handler) {
    if (command_num < 0 || command_num >= NUM_SYS_CALLS) {
        panic("couldn't register syscall");
    }
    if (sys_calls[command_num]) {
        panic("syscall command number already in use");
    }
    sys_calls[command_num] = handler;
}

void *intr_80h_handler(int command, struct interrupt_frame *frame) {
    void *res = 0;

    // DESIGN NOTE:
    // This will switch to the kernel segments(privliged) and then to the kernel
    // page table Switching to kernel pape table is not actually needed because
    // the kernel is already mapped in the user page table. User Memory layout:
    //  Everything mapped identically to phyiscal memory
    //  Except for addresses above "config.h/KHEAP_SAFE_BOUNDARY"
    //  Addresses above this will be accessable by the user space.
    //  All the kernel space is mapping to <= KHEAP_SAFE_BOUNDARY
    // NOTE: kernel_va_switch changed to not load kernel's cr3 for now
    kernel_va_switch();

    task_save_current_state(frame);

    res = syscall_handle_command(command, frame);

    // Get back to the user land pages
    task_page();

    return res;
}

void intr_generic_handler() {
    port_io_out_byte(MASTER_PIC_PORT, MASTER_PIC_INTR_ACK);
}

void idt_set(int intr_num, void *addr) {
    struct idt_entry *entry = &idt[intr_num];
    entry->base_lo = (uint32_t)addr & 0x0000FFFF;
    entry->sel = KERNEL_CODE_SEGMENT;
    entry->always0 = 0;

    // last E -> interrupt gate
    // first E -> 1110 -> Present (1) (Set), DPL(11) (ring 3), S (0) (interrupt
    // gate)
    entry->flags = 0xEE;

    entry->base_hi = ((uint32_t)addr & 0xFFFF0000) >> 16;
}

void idt_init() {

    memset(&idt, 0, sizeof(idt));
    memset(&sys_calls, 0, sizeof(sys_calls));
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;

    cli_count = 0;

    for (int i = 0; i < NUM_INTERRUPTS; i++) {
        idt_set(i, int_generic_h);
    }

    idt_set(0, int0h);
    idt_set(0x21, int21h);
    idt_set(0x80, int80h);

    idt_load(&idtp);
}

// --------- TESTS ------------- //

extern void test_int0();
extern void test_div0();

static void test_div_by_zero() { test_div0(); }

static void test_div_by_zero_int0() { test_int0(); }

void external_interrupts_test() {
    enable_interrupts();
    idt_set(0x20, int21h);
}

void idt_test() {
    test_div_by_zero_int0();
    test_div_by_zero();
    test_div_by_zero_int0();
}
