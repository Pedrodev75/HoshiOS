[BITS 32]

global timer_irq
extern timer_tick

section .text

timer_irq:
    pusha

    call timer_tick

    mov al, 0x20
    out 0x20, al

    popa
    iretd