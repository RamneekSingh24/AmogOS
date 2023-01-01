section .asm

extern intr_0_handler
extern intr_21_handler
extern intr_80h_handler
extern intr_generic_handler

global int0h, int21h, int_generic_h
global enable_interrupts, disable_interrupts
global int80h


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



int80h:
    cli
    ; INTERRUPT FRAME START
    ; ALREADY PUSHED TO US BY THE PROCESSOR WHEN `int x` is called
    ; uint32_t ip
    ; uint32_t cs;
    ; uint32_t flags
    ; uint32_t sp;
    ; uint32_t ss;
    ; Pushes the general purpose registers to the stack
    pushad
    
    ; INTERRUPT FRAME END

    ; int isr80h_handler(int command, struct interrupt_frame *frame)
    ; We need to pass the command and the interrupt frame to the handler
    ; Push the stack pointer so that we are pointing to the interrupt frame
    push esp
    ; EAX holds our command lets push it to the stack for isr80h_handler
    push eax
    call intr_80h_handler

    mov dword[tmp_res], eax
    add esp, 8

    ; Restore general purpose registers for user land
    popad
    mov eax, [tmp_res]
    iretd




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




section .data
tmp_res: dd 0