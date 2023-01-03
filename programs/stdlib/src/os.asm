[BITS 32]

section .asm

global print:function
global get_key:function
global put_char:function
global mmap:function
global munmap:function
global cls:function
global create_proccess:function

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


;int mmap(void* va_start, void* va_end, int flags);
mmap:
    push ebp
    mov ebp, esp

    push dword[ebp+8] ; va_start
    push dword[ebp+12] ; va_end
    push dword[ebp+16] ; flags
    mov eax, 4 ; mmap syscall 
    int 0x80
    add esp, 12 ; pop va_start, va_end, flags

    pop ebp
    ret

;int munmap(void* va_start);
munmap:
    push ebp
    mov ebp, esp

    push dword[ebp+8] ; va_start
    mov eax, 5 ; munmap syscall 
    int 0x80
    add esp, 4 ; pop va_start

    pop ebp
    ret

;void clear_screen();
cls:
    push ebp
    mov ebp, esp

    mov eax, 6 ; clear_screen syscall 
    int 0x80

    pop ebp
    ret

; int create_proccess(const char* file_path, int argc, char** argv);
create_proccess:
    push ebp
    mov ebp, esp

    push dword[ebp+8] ; file_path
    push dword[ebp+12] ; argc
    push dword[ebp+16] ; argv
    mov eax, 7 ; create_proccess syscall 
    int 0x80
    add esp, 12 ; pop file_path, argc, argv

    pop ebp
    ret