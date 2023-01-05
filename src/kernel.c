#include "kernel.h"
#include "config.h"
#include "console/console.h"
#include "dev/keyboard.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "syscall/syscall.h"
#include "task/process.h"
#include "task/tss.h"

#include <stddef.h>
#include <stdint.h>

void panic(char *msg) {
    println(msg);
    while (1) {
    };
}

struct tss tss;
struct gdt gdt_real[TOTAL_GDT_SEGS];
struct gdt_structured gdt_structured[TOTAL_GDT_SEGS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},         // NULL
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9a},   // Kernel code  segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92},   // Kernel data segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xf8},   // User code segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xf2},   // User data segment
    {.base = 0x00, .limit = sizeof(tss), .type = 0xE9}}; // TSS

void tss_init() {
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SEGMENT;
    tss_load(0x28);
}

void gdt_init() {
    // Set the addr of tss, which couldn't be set at compile time
    gdt_structured[TOTAL_GDT_SEGS - 1].base = (uint32_t)&tss;
    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, TOTAL_GDT_SEGS);
    gdt_load(gdt_real, sizeof(gdt_real));
}

void kernel_registers();

// Switches to kernel privlidged segments
// DOES NOT SWITCH PAGE TABLES
void kernel_va_switch() {
    kernel_registers();
    // DESIGN NOTE: No need to load kernel page table here, kernel is already
    // mapped paging_load_kernel_page_table();
}

void kernel_main() {

    console_init();
    gdt_init();
    tss_init();
    kheap_init();
    kpaging_init();
    fs_init();
    disk_init();
    idt_init();
    procs_init();
    keyboard_init();
    register_syscalls();

    println("Loading first proc..");

    struct process *proc = 0;
    int res = process_new("0:/shell", &proc);
    if (res != STATUS_OK) {
        print("Err code: ");
        print_int(res);
        panic("\nFailed to load shell");
    }

    process_add_arguments(proc, 3, 22, "0:/shell\0Amogos\0Shell\0");

    println("Loading success! Starting first proc..");

    task_run_init_task();

    // print("\n");
    // print("bb");
    // print("cc");
    // print("\n");
    // println("");
    // disk_streamer_test();
    // test_fs_utils();
    // test_paging_set();
    // kheap_test();
    // console_test();
    // idt_test();
    // io_test();
    // fs_test();

    // interrupts disabled on here
    // external_interrupts_test:
    // enables interrupts and maps keyboard handler to timer interrupt handler
    // should see keyboard pressed again and again due to timer interrupt
    // external_interrupts_test();
    // ^^ enables interrupts
    while (1) {
    };
}
