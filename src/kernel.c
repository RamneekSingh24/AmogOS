#include "kernel.h"
#include "console/console.h"
#include "idt/idt.h"

#include <stdint.h>


void test_console() {
    print("Hello World!\n");
    print("Hello World!\n");
}




void kernel_main() {
    console_init();

    test_console();
    
    idt_init();

    idt_test();




}
