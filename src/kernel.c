#include "kernel.h"
#include "console/console.h"
#include "memory/heap/kheap.h"
#include "idt/idt.h"
#include "io/io.h"

#include <stdint.h>
#include <stddef.h>

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


    void* ptr = kmalloc(50);
    void* ptr2 = kmalloc(5000);
    void* ptr3 = kmalloc(5300);
    kfree(ptr2);
    void* ptr4 = kmalloc(15000);

    if (ptr || ptr2 || ptr3 || ptr4) {};

    // print_int((int)ptr4);

}
