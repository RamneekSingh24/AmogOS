[BITS 32]

section .asm


global paging_load_dir, paging_enable

paging_load_dir:
    push ebp
    mov ebp, esp
    mov eax, [ebp +  8]
    mov cr3, eax
    pop ebp
    ret


paging_enable:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    ret