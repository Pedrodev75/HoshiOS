#include "panic.h"
#include "vga.h"
#include "utypes.h"

void kernel_panic(const char *message) {
    __asm__ volatile ("cli");

    vga_clear();
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);

    vga_print("KERNEL PANIC");
    vga_newline(2);

    vga_print(message);
    vga_newline(2);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void kernel_panic_addr(const char *message, uint32_t address) {
    __asm__ volatile ("cli");

    vga_clear();
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);

    vga_print("KERNEL PANIC");
    vga_newline(2);

    vga_print(message);
    vga_print(", address: 0x");
    vga_print_hex(address);
    vga_newline(2);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}