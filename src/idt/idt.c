#include "idt.h"
#include "config.h"
#include "console/console.h"
#include "io/io.h"
#include "kernel.h"
#include "memory/memory.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"

// assembly wrappers for all the interrupt handlers
// NOTE: some of these such as sys_call(0x80) are overridden
extern void *interrupt_handler_asm_wrappers[NUM_INTERRUPTS];

// NOTE: some of these such as sys_call(0x80) are overridden and not used from
// here
static INTERRUPT_CALL_BACK interrupt_call_backs[NUM_INTERRUPTS];

extern void idt_load(struct idt_ptr *ptr);
extern void int80h();
extern void enable_interrupts();
extern void disable_interrupts();

struct idt_entry idt[NUM_INTERRUPTS];
struct idt_ptr idtp;

volatile int cli_count;

void pop_cli() {
    if (cli_count <= 0) {
        println("error pop cli with count already <= 0");
    } else {
        cli_count--;
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

void idt_handle_exception(struct interrupt_frame *frame) {
    process_exit(task_current()->proc, -STATUS_PROC_EXCEPTION);
    task_switch_and_run_any();
}

int idt_register_interrupt_call_back(int interrupt_no,
                                     INTERRUPT_CALL_BACK call_back) {
    if (interrupt_no < 0 || interrupt_no >= NUM_INTERRUPTS || !call_back) {
        return -STATUS_INVALID_ARG;
    }
    interrupt_call_backs[interrupt_no] = call_back;
    return 0;
}

void interrupt_handler(int interrupt_no, struct interrupt_frame *frame) {

    // kernel_va_switch(); not needed kernel is already mapped

    if (interrupt_no == 0xE) {
        println("page fault");
    }

    if (interrupt_call_backs[interrupt_no] != 0) {
        interrupt_call_backs[interrupt_no](frame);
    } else {
        // panic("Unhandled interrupt");
        // TODO: check if we actually have to ack based on interrupt_no
        port_io_out_byte(MASTER_PIC_PORT, MASTER_PIC_INTR_ACK);
    }

    // DESIGN NOTE: interrupt call backs should be responsible for acking the
    // interrupt We may never get back here in time as the callback may switch
    // to another task port_io_out_byte(MASTER_PIC_PORT, MASTER_PIC_INTR_ACK);

    // task_page(); // not needed as we are not switching to kernel page table
}

void intr_21_handler() {
    print("Keyboard pressed\n");
    port_io_out_byte(MASTER_PIC_PORT, MASTER_PIC_INTR_ACK);
}

static SYSCALL_HANDLER sys_calls[NUM_SYS_CALLS];

void *syscall_handle_command(int command, struct interrupt_frame *frame) {
    if (command < 0 || command >= NUM_SYS_CALLS) {
        return (void *)-SYSCALL_NOT_IMPLEMENTED;
    }

    SYSCALL_HANDLER sys_call = sys_calls[command];
    if (!sys_call) {
        return (void *)-SYSCALL_NOT_IMPLEMENTED;
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
    // kernel_va_switch();

    task_save_current_state(frame);

    res = syscall_handle_command(command, frame);

    // Get back to the user land pages
    // task_page();

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

    memset(idt, 0, sizeof(idt));
    memset(sys_calls, 0, sizeof(sys_calls));
    memset(interrupt_call_backs, 0, sizeof(interrupt_call_backs));
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;

    cli_count = 0;

    for (int i = 0; i < NUM_INTERRUPTS; i++) {
        idt_set(i, interrupt_handler_asm_wrappers[i]);
    }

    for (int i = 0; i < 0x20; i++) {
        idt_register_interrupt_call_back(i, idt_handle_exception);
    }

    idt_set(0x80, int80h);

    idt_load(&idtp);
}

// --------- TESTS ------------- //

extern void test_int0();
extern void test_div0();

static void test_div_by_zero() { test_div0(); }

static void test_div_by_zero_int0() { test_int0(); }

void external_interrupts_test() { enable_interrupts(); }

void idt_test() {
    test_div_by_zero_int0();
    test_div_by_zero();
    test_div_by_zero_int0();
}
