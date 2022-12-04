section .asm


global port_io_input_byte
global port_io_input_word
global port_io_out_byte
global port_io_out_word


port_io_input_byte:
    push ebp
    mov  ebp, esp
    
    mov edx, [ebp +  8] ; port from argument
    xor eax, eax   ; setup return value

    in al, dx      ; lower 8 bits of eax set by in instruction
    
    pop ebp
    ret


port_io_input_word:
    push ebp
    mov ebp, esp

    mov edx, [ebp +  8]
    xor eax, eax

    in ax, dx

    pop ebp
    ret


port_io_out_byte:
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8] ; port
    mov eax, [ebp + 12] ; data
    
    out dx, al

    pop ebp
    ret


port_io_out_word:
    push ebp
    mov ebp, esp

    mov edx, [ebp + 8] ; port
    mov eax, [ebp + 12] ; data

    out dx, ax

    pop ebp
    ret