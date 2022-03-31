#include "idt.h"
#include "config.h"
#include "memory/memory.h"
#include "kernel.h"
#include "io/io.h"


struct idt_desc idt_descriptors[TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void idt_load(struct idtr_desc* ptr);
extern void int21h();
extern void no_interrupt();

void int21h_handler() {
    print("keyboard pressed \n");
    out_byte(0x20, 0x20);
}


void idt_set(int intr_no, void* address) {
    struct idt_desc* desc = &idt_descriptors[intr_no];
    desc->offset_1 = (uint32_t) address & 0x0000ffff;
    desc->offset_2 = (uint32_t) address >> 16;
    desc->selector = CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE; // 0b1110 -> interrupt gate
                            // storage seg = 0
                            // DPL = 3
}

void idt_zero() {
    print("Divide by zero error\n");
}



// IRQ0 = INT20
void idt_timer() {

}

void no_interrupt_handler() {
    out_byte(0x20, 0x20);
}


void idt_init() {
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t) idt_descriptors;
    
    for (int i = 0; i < TOTAL_INTERRUPTS; i++) {
        idt_set(i, no_interrupt);
    }

    idt_set(0, idt_zero);
    idt_set(0x21, int21h);
    
    idt_load(&idtr_descriptor);

}


