section .asm


global idt_load, test_int0, test_div0

idt_load:         ; first argument idtr_ptr
    push ebp      ; Calling conventions for compatibility with C
    mov ebp, esp

    mov ebx, [ebp + 8] ; First argument
    lidt [ebx]

    pop ebp
    ret


test_int0:
    int 0
    ret

test_div0:
    mov eax, 0
    div eax
    ret