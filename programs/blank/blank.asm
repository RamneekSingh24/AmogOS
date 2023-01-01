[BITS 32]

label:
    push message
    push 30
    mov eax, 1
    int 0x80
    add esp, 8
    jmp label

section .data
message db "Hello kernel, I am blank.bin", 0x00