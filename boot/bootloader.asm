; Based on code by lowwryzen.
; Original source: https://github.com/lowwryzen/laurix
; Licensed under the MIT License.
; Modified for HoshiOS.
[BITS 16]
[ORG 0x7C00]

KERNEL_LOCATION equ 0x1000
DATA_SEG equ 0x10
CODE_SEG equ 0x08

; Quantos setores de 512 bytes o bootloader vai ler do disco para o kernel.
;
; LIMITE IMPORTANTE: o kernel é carregado a partir de KERNEL_LOCATION (0x1000)
; e cresce para cima na memória. O próprio bootloader (este código) e a pilha
; (SP = 0x7C00) vivem em 0x7C00-0x7E00 e ainda estão "em uso" enquanto o
; disco é lido. Se KERNEL_SECTORS*512 ultrapassar (0x7C00 - 0x1000), o
; kernel carregado sobrescreve o bootloader NO MEIO da própria leitura,
; corrompendo o código que ainda vai ser executado (jc/lgdt/jmp abaixo).
; Por isso o teto seguro é 48 setores (24KB), com folga de ~2.5KB.
; Se mudar aqui, mude também MAX_KERNEL_SIZE no makefile.
KERNEL_SECTORS equ 48   ; 48 * 512 = 24 KB de kernel (teto seguro)

start:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl
    sti

    in al, 0x92
    or al, 00000010b
    out 0x92, al

    call detect_memory

load_kernel:
    mov si, dap

    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc load_kernel_error

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode_entry

    E820_COUNT_ADDRESS equ 0x7FF0
    E820_BUFFER_SEGMENT equ 0x8000
    E820_ENTRY_SIZE     equ 24
    E820_MAX_ENTRIES    equ 128

gdt_start:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

dap:
    db 0x10             ; size
    db 0
    dw KERNEL_SECTORS   ; número de setores a ler (era fixo em 2 = 1KB)
    dw KERNEL_LOCATION
    dw 0x0000
    dq 1                ; LBA sector

[BITS 32]
protected_mode_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000
    jmp KERNEL_LOCATION

[BITS 16]
load_kernel_error:
    mov ah, 0x0E
    mov al, '1'
    int 0x10
    jmp $

detect_memory:
    pushad
    push ds
    push es

    ; DS = 0 para acessar E820_COUNT_ADDRESS como endereço físico.
    xor ax, ax
    mov ds, ax
    mov word [E820_COUNT_ADDRESS], 0

    ; ES:DI aponta inicialmente para 0x8000:0x0000 = 0x80000.
    mov ax, E820_BUFFER_SEGMENT
    mov es, ax
    xor di, di

    ; EBX = 0 indica que esta é a primeira chamada E820.
    xor ebx, ebx

.next_entry:
    ; Impede que a BIOS escreva mais entradas do que o buffer suporta.
    cmp word [E820_COUNT_ADDRESS], E820_MAX_ENTRIES
    jae .done

    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, E820_ENTRY_SIZE

    ; Habilita o campo de atributos estendidos quando suportado.
    mov dword [es:di + 20], 1

    int 0x15

    ; Carry ligado indica falha ou fim não convencional do mapa.
    jc .done

    ; A BIOS precisa devolver a assinatura "SMAP".
    cmp eax, 0x534D4150
    jne .done

    ; Uma entrada E820 precisa ter pelo menos os 20 bytes obrigatórios.
    cmp ecx, 20
    jb .done

    ; Ignora entradas cujo tamanho seja zero.
    mov eax, dword [es:di + 8]
    or eax, dword [es:di + 12]
    jz .continue

    inc word [E820_COUNT_ADDRESS]
    add di, E820_ENTRY_SIZE

.continue:
    ; EBX diferente de zero indica que ainda existem entradas.
    test ebx, ebx
    jnz .next_entry

.done:
    pop es
    pop ds
    popad
    ret

boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xAA55	 