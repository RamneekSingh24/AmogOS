section .asm

extern intr_0_handler
extern intr_21_handler
extern intr_generic_handler

global int0h, int21h, int_generic_h
global enable_interrupts, disable_interrupts


global idt_load, test_int0, test_div0



idt_load:         ; first argument idtr_ptr
    push ebp      ; Calling conventions for compatibility with C
    mov ebp, esp

    mov ebx, [ebp + 8] ; First argument
    lidt [ebx]

    pop ebp
    ret


; Interrupt wrappers 

int0h:
    cli    ; disable interrupts to prevent nested interrupts

    pushad ; save all general purpose registers
    call intr_0_handler
    popad  ; restore gp registers 
    
    sti
    iret   ; special instruction to return from interrupt and go back to where we were 




int21h:
    cli    ; disable interrupts to prevent nested interrupts

    pushad ; save all general purpose registers
    call intr_21_handler
    popad  ; restore gp registers 
    
    sti
    iret   ; special instruction to return from interrupt and go back to where we were 


; A generic interupt handler
int_generic_h:

    cli    ; disable interrupts to prevent nested interrupts

    pushad ; save all general purpose registers
    call intr_generic_handler
    popad  ; restore gp registers 
    
    sti
    iret   ; special instruction to return from interrupt and go back to where we were 


disable_interrupts:
    cli
    ret

enable_interrupts:
    sti
    ret



test_int0:
    int 0
    ret

test_div0:
    mov eax, 0
    div eax
    ret

