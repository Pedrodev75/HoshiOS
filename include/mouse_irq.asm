[BITS 32]

global mouse_irq
extern mouse_irq_handler

section .text

mouse_irq:
    pusha

    call mouse_irq_handler

    mov al, 0x20
    out 0xA0, al
    out 0x20, al

    popa
    iretd