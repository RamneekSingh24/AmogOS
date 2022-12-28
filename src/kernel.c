#include "kernel.h"
#include "config.h"
#include "console/console.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
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
    // Set the addr of tss, don't which couldn't be set a compile time
    gdt_structured[TOTAL_GDT_SEGS - 1].base = (uint32_t)&tss;
    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, TOTAL_GDT_SEGS);
    gdt_load(gdt_real, sizeof(gdt_real));
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
