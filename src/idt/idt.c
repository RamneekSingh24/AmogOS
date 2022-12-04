#include "idt.h"
#include "config.h"
#include "memory/memory.h"
#include "console/console.h"

extern void idt_load(struct idt_ptr* ptr);


struct idt_entry idt[NUM_INTERRUPTS];
struct idt_ptr idtp;


void intr_0_handler() {
    print("Divide by zero exception\n");
}


void idt_set(int intr_num, void* addr) {
    struct idt_entry* entry = &idt[intr_num];
    entry->base_lo = (uint32_t) addr & 0x0000FFFF;
    entry->sel = KERNEL_CODE_SEGMENT;
    entry->always0 = 0;

    // last E -> interrupt gate
    // first E -> 1110 -> Present (1) (Set), DPL(11) (ring 3), S (0) (interrupt gate)
    entry->flags = 0xEE;   


    entry->base_hi = ((uint32_t) addr & 0xFFFF0000) >> 16;
}


void idt_init() {

    memset(&idt, 0, sizeof(idt));
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t) &idt;


    idt_set(0, intr_0_handler);

    idt_load(&idtp);

}


// --------- TESTS ------------- //

extern void test_int0();
extern void test_div0();

static void test_div_by_zero() {
    test_div0();
}

static void test_div_by_zero_int0() {
    test_int0();
}

void idt_test() {
    test_div_by_zero_int0();
    test_div_by_zero();
    test_div_by_zero_int0();
}
