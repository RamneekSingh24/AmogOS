[BITS 32]

section .asm

_start:
    push message
    push 100
    mov eax, 1
    int 0x80
    add esp, 8; pop message, pop 30
_repeat:
    call getkey
    call putkey

    jmp _repeat

getkey:
    mov eax, 2
    int 0x80
    cmp eax, 0
    je getkey
    ret

putkey:
    push eax
    mov eax, 3
    int 0x80
    add esp, 4
    ret
    
section .data
message db "Greetings, you are inside init proc terminal", 0x0a, 0x00