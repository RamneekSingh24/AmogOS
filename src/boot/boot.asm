ORG 0x7c00 ; BIOS loads the boot sector at 0x7c00
BITS 16 
;processor starts in real mode (16-bit) model of intel 8086 
;for compatibility with old operating systems

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

_start:
    jmp short start ; BIOS parameter block
    nop

 times 33 db 0 ; BIOS parameter block

; start:
;     jmp 0:start2

start:
    ; set up 8086's memory segments(1 MB) 
    cli
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00 ; BIOS loads the boot sector at 0x7c00
    sti 

    ; setup & switch to protected mode (32 bit) (4 GB)
    cli
    lgdt[gdt_descriptor] ; setting gdt is necessary to 
                         ; switch to 32 bit mode 
    mov eax, cr0   
    or eax, 0x1     
    mov cr0, eax ; Set protected mode bit to switch
    jmp CODE_SEG:load32
   ; jmp $


; Global descriptor table(stores memory segments)
; GDT contains base, bound, protection & ring level 
; access rights to segments
; We will just be setting default values, since we will 
; use paging instead of segmentation

gdt_start:
; GDT entries

; first entry must be null
gdt_null:
    dd 0x0
    dd 0x0

gdt_code:       ;Default values 
    dw 0xffff
    dw 0
    db 0
    db 0x9a ;Access Bytes
    db 11001111b
    db 0

gdt_data:
    dw 0xffff
    dw 0
    db 0
    db 0x92 ;Access Bytes
    db 11001111b
    db 0

gdt_end:


gdt_descriptor: 
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32] ; Code for protected mode, 32 bit
load32:
    ; load kernel into memory
    mov eax, 1    ; start of the kernel sector
                  ; (0th sector is the boot sector)
    mov ecx, 100  ; number of sectors to read
    mov edi, 0x0100000 ; load kernel at 1 MB
    call ata_lba_read
    jmp CODE_SEG:0x0100000

; small ATA driver to read kernel from disk
ata_lba_read:
    mov ebx, eax ; Backup LBA
    ; send highest 8 bits of lba to hard disk controller
    shr eax, 24 
    or eax, 0xE0 ; Select master drive 
    mov dx, 0x1F6
    out dx, al
    ; sent to port 0x1F6

    ; send total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ;done

    ; send rest of bits of lba to HDisk controller
    mov eax, ebx
    mov dx, 0x1F3
    out dx, al


    mov eax, ebx
    mov dx, 0x1F4
    shr eax, 8
    out dx, al


    mov dx, 0x1F5
    mov eax, ebx
    shr eax, 16
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al


.next_sector:
    push ecx


; keep polling until done
.poll:
    mov dx, 0x1f7
    in al, dx
    test al, 8
    jz .poll

; Read 256 words at a time from io port to memory
    mov ecx, 256
    mov dx, 0x1F0
    rep insw ; io port to memory
    pop ecx
    loop .next_sector

    ret


times 510-($-$$) db 0
dw 0xAA55 ; boot signature(at 511 and 512 byte)

