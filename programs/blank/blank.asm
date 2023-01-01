[BITS 32]

label:
    mov eax, 0 
    mov ebx, 13
    mov edx, 13
    int 0x80
    jmp label