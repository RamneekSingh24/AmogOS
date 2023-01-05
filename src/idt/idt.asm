section .asm

extern intr_80h_handler
extern interrupt_handler
extern interrupt_handler_asm_wrappers

global idt_load, enable_interrupts, disable_interrupts
global int80h


global test_int0, test_div0



idt_load:         ; first argument idtr_ptr
    push ebp      ; Calling conventions for compatibility with C
    mov ebp, esp

    mov ebx, [ebp + 8] ; First argument
    lidt [ebx]

    pop ebp
    ret


; Interrupt wrappers 



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


%macro interrupt 1
    global int%1
    int%1:
        ; INTERRUPT FRAME START
        ; ALREADY PUSHED TO US BY THE PROCESSOR UPON ENTRY TO THIS INTERRUPT
        ; uint32_t ip
        ; uint32_t cs;
        ; uint32_t flags
        ; uint32_t sp;
        ; uint32_t ss;
        ; Pushes the general purpose registers to the stack
        pushad
        ; Interrupt frame end
        push esp
        push dword %1
        call interrupt_handler
        add esp, 8
        popad
        iret
%endmacro

%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep




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


%macro interrupt_array_entry 1
    dd int%1
%endmacro

interrupt_handler_asm_wrappers:
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep