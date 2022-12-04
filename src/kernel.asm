[BITS 32]
global _start
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ;Enable A20 line
    in al, 0x92
    or al, 2
    out 0x92, al


    ;Remap master PIC, 8-15 are used for exceptions in protected mode.

    ; master PIC can be programmed at port number 0x20, 0x21
    mov al, 00010001b
    out 0x20, al     ; init the master PIC, 
    mov al, 0x20     ; int number 0x20 is mapped to IRQ0)
    out 0x21, al

    ; switch to x86 mode 
    mov al, 00000001b     
    out 0x21, al

    ; End Remap master PIC

    call kernel_main
    jmp $



; alignment needed for C code that follows
times 512-($-$$) db 0 
