#ifndef IDT_H
#define IDT_H

#include "utypes.h"
#include "vga.h"

extern void load_IDT();
extern void set_idt_entry(void *isr, uint8_t flags, uint8_t index);

extern void ir0();
extern void ir6();
extern void ir13();
extern void ir14();
extern void ir8();

void division_error_handler();
void invalid_opcode_handler();
void general_protection_fault_handler();
void page_fault_handler();
void double_fault_handler();
void clear_IDT(void);


#endif