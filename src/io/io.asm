section .asm




global in_byte, in_word, out_byte, out_word


in_byte:
    push ebp
    mov ebp, esp
    xor eax, eax  
    mov edx, [ebp+8]
    in al, dx
    pop ebp
    ret

in_word:
    push ebp
    mov ebp, esp
    xor eax, eax
    mov edx, [ebp+8]
    in ax, dx
    pop ebp
    ret

out_byte:
    push ebp
    mov ebp, esp
    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, al
    pop ebp
    ret

out_word:
    push ebp
    mov ebp, esp
    mov eax, [ebp+12]
    mov edx, [ebp+8]
    out dx, ax
    pop ebp
    ret