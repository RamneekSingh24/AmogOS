#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Interrupt Descriptor Table Entry
struct idt_entry {
    uint16_t base_lo; // The lower 16 bits of the address to jump to when this interrupt fires.
    uint16_t sel;     // Kernel segment selector.
    uint8_t  always0; // This must always be zero.
    uint8_t  flags;   // Descriptor type and attributes.
    uint16_t base_hi; // The upper 16 bits of the address to jump to.
} __attribute__((packed));

// Interrupt Descriptor Table Pointer
struct idt_ptr {
    uint16_t limit; // Size of IDT - 1
    uint32_t base;  // Base addr of the start of IDT
} __attribute__((packed));



void idt_test();
void idt_init();
void external_interrupts_test();

#endif