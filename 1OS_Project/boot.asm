[org 0x7c00]
[bits 16]

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; 1. Clear Screen via BIOS
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; 2. Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 3. Setup 64-Bit Paging Tables (Inside safe memory 0x9000)
    mov edi, 0x9000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd       ; Clear with zeros

    mov dword [0x9000], 0xA003
    mov dword [0xA000], 0xB003
    mov dword [0xB000], 0x0083 ; Identity Map 2MB page entry

    ; 4. Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 5. Enable Long Mode inside EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 6. Enable Paging and Protected Mode
    mov eax, cr0
    or eax, 1 << 31 | 1 << 0
    mov cr0, eax

    ; 7. Load 64-Bit Global Descriptor Table (GDT)
    lgdt [gdt_ptr]

    ; 8. Far jump directly into pure 64-Bit Execution Long Mode!
    jmp 0x08:init_64bit

[bits 64]
init_64bit:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; 💥 1OS 64-BIT CANVAS DESIGN 💥
    mov rdi, 0xB8000 ; VGA Color frame buffer address
    
    ; Step A: Poori screen par Dark Blue background failao (80x25 characters)
    mov rcx, 1000
    mov rax, 0x1F201F201F201F20 
    rep stosq

    ; Step B: Top Border Draw (=======)
    mov rdi, 0xB8000
    mov rcx, 40
    mov rax, 0x9F3D9F3D9F3D9F3D 
    rep stosq

    ; Step C: Bottom Border Draw (=======)
    mov rdi, 0xB8F00 
    mov rcx, 40
    mov rax, 0x9F3D9F3D9F3D9F3D 
    rep stosq

    ; Step D: Row 2, Column 3 par "1OS: 64-Bit Microkernel Canvas" print karo
    mov rdi, 0xB8000 + 326
    mov dword [rdi],    0x1F4F1F31 ; '1','O'
    mov dword [rdi+4],  0x1F3A1F53 ; 'S',':'
    mov dword [rdi+8],  0x1F361F20 ; ' ', '6'
    mov dword [rdi+12], 0x1F2D1F34 ; '4','-'
    mov dword [rdi+16], 0x1F691F42 ; 'B','i'
    mov dword [rdi+20], 0x1F201F74 ; 't',' '
    mov dword [rdi+24], 0x1F4D1F20 ; ' ', 'M'
    mov dword [rdi+28], 0x1F631F69 ; 'i','c'
    mov dword [rdi+32], 0x1F6F1F72 ; 'r','o'
    mov dword [rdi+36], 0x1F651F6B ; 'k','e'
    mov dword [rdi+40], 0x1F6E1F72 ; 'r','n'
    mov dword [rdi+44], 0x1F6C1F65 ; 'e','l'
    mov dword [rdi+48], 0x1F201F20 ; ' ', ' '
    mov dword [rdi+52], 0x1F611F43 ; 'C','a'
    mov dword [rdi+56], 0x1F761F6E ; 'n','v'
    mov dword [rdi+60], 0x1F731F61 ; 'a','s'

    ; Step E: Light Green color mein "[ INITIALIZED ]" tag rendering
    mov rdi, 0xB8000 + 434
    mov dword [rdi],    0x1A491F5B ; '[', 'I'
    mov dword [rdi+4],  0x1A491A4E ; 'N','I'
    mov dword [rdi+8],  0x1A491A54 ; 'T','I'
    mov dword [rdi+12], 0x1A4C1A41 ; 'A','L'
    mov dword [rdi+16], 0x1A5A1A49 ; 'I','Z'
    mov dword [rdi+20], 0x1A441A45 ; 'E','D'
    mov dword [rdi+24], 0x1F5D1A20 ; ' ', ']'

lock_system:
    cli
    hlt
    jmp lock_system

; --- 64-Bit GDT (Global Descriptor Table) Layout ---
align 8
gdt_start:
    dq 0x0000000000000000 ; Null descriptor block
gdt_code:
    dq 0x00209A0000000000 ; Code Segment register mapping
gdt_data:
    dq 0x0000920000000000 ; Data Segment register mapping
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dq gdt_start

; Exactly 512 bytes MBR standard layout structure
times 510 - ($ - $$) db 0
dw 0xAA55