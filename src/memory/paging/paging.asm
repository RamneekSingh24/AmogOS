[BITS 32]

section .asm

global paging_load_directory, enable_paging

paging_load_directory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    mov eax, 0x80000000
    mov cr0, eax
    pop ebp
    ret