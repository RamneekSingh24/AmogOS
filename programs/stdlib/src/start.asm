[BITS 32]

global _start
extern main

section .asm
_start:
    call main
    ret




; putkey:
;     push eax
;     mov eax, 3
;     int 0x80
;     add esp, 4
;     ret
    
; section .data
; message db "Greetings, you are inside init proc terminal", 0x0a, 0x00