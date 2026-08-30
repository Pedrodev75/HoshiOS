#include "idt.h"
#include "kernel/panic.h"

extern char __idt_start__;

void clear_IDT(void) {
    volatile uint8_t *idt =
        (volatile uint8_t *)&__idt_start__;

    for (uint32_t i = 0; i < 256 * 8; i++) {
        idt[i] = 0;
    }
}

void division_error_handler() {
    kernel_panic("Zero division error");
}

void invalid_opcode_handler() {
    kernel_panic("Invalid opcode error");
}

 void general_protection_fault_handler() {
    kernel_panic("General protection fault error");
}

void page_fault_handler() {
    uint32_t fault_address;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_address));

    kernel_panic_addr("Page fault error", fault_address);
}

void double_fault_handler() {
    kernel_panic("Double fault error");
}