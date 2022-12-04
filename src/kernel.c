#include "kernel.h"
#include "console/console.h"

#include <stdint.h>


void test_print() {
    print("Hello World!\n");
    print("Hello World!\n");
}


void kernel_main() {
    console_init();

    test_print();
    
    



}
