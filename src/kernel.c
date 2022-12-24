#include "kernel.h"
#include "console/console.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"

#include <stddef.h>
#include <stdint.h>

void console_test() {
    print("Hello World!\n");
    print("Hello World!\n");
}

void kernel_main() {

    console_init();

    console_test();

    kheap_init();

    idt_init();

    // idt_test();
    // io_test();

    // interrupts disabled on here

    // external_interrupts_test:
    // enables interrupts and maps keyboard handler to timer interrupt handler
    // should see keyboard pressed again and again due to timer interrupt
    // external_interrupts_test();

    kpaging_init();
    test_paging_set();

    kheap_test();

    print("\n");
    print("bb");
    print("cc");
}
