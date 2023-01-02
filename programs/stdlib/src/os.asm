[BITS 32]

global print:function
global get_key:function
global put_char:function

; void print(const char* str, int len)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]  ; str pointer
    push dword[ebp+12] ; len
    mov eax, 1
    int 0x80
    add esp, 8 ; pop str and len

    pop ebp
    ret


; int get_key();
get_key:
    push ebp
    mov ebp, esp
    
    mov eax, 2
    int 0x80 ; sets eax to the key pressed
  
    ; return type is int, so eax is the return value, no need to do anything
    pop ebp
    ret

; void put_char(int c)
put_char:
    push ebp
    mov ebp, esp

    push dword[ebp+8] ; char c
    mov eax, 3
    int 0x80
    add esp, 4 ; pop c

    pop ebp
    ret