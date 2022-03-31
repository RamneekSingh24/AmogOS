section .asm

global idt_load, int21h, enable_interrupts, disable_interrupts
global no_interrupt

extern int21h_handler,no_interrupt_handler 

idt_load:
    push ebp
    mov ebp, esp
    mov ebx, [ebp+8]
    lidt [ebx]
    pop ebp
    ret


int21h:
    cli
    pushad
    call int21h_handler
    popad
    sti
    iret



enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret


no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret