#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Interrupt Descriptor Table Entry
struct idt_entry {
    uint16_t base_lo; // The lower 16 bits of the address to jump to when this
                      // interrupt fires.
    uint16_t sel;     // Kernel segment selector.
    uint8_t always0;  // This must always be zero.
    uint8_t flags;    // Descriptor type and attributes.
    uint16_t base_hi; // The upper 16 bits of the address to jump to.
} __attribute__((packed));

// Interrupt Descriptor Table Pointer
struct idt_ptr {
    uint16_t limit; // Size of IDT - 1
    uint32_t base;  // Base addr of the start of IDT
} __attribute__((packed));

struct interrupt_frame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved; // esp pushed by pushad (DO NOT USE)
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp; // actual esp pushed by the processor during interrupt
    uint32_t ss;
} __attribute__((packed));

typedef void*(*SYSCALL_HANDLER)(struct interrupt_frame* frame);

void syscall_register_command(int command_num, SYSCALL_HANDLER hanlder);

void idt_test();
void idt_init();
void external_interrupts_test();

#endif