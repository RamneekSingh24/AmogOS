#include "kernel.h"
#include "console/console.h"
#include "disk/disk.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "fs/utils.h"

#include <stddef.h>
#include <stdint.h>

void console_test() {
    print("Hello World!\n");
    print("Hello World!\n");
}

void kernel_main() {

    console_init();
    kheap_init();
    kpaging_init();
    disk_init();
    idt_init();

    // print("\n");
    // print("bb");
    // print("cc");
    // print("\n");
    // println("");
    
    // test_fs_utils();
    // test_paging_set();
    // kheap_test();
    // console_test();
    // idt_test();
    // io_test();

    // interrupts disabled on here
    // external_interrupts_test:
    // enables interrupts and maps keyboard handler to timer interrupt handler
    // should see keyboard pressed again and again due to timer interrupt
    // external_interrupts_test();
    // ^^ enables interrupts
}
