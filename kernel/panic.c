#include "panic.h"
#include "vga.h"
#include "utypes.h"
#include "drivers/serial/serial.h"

void kernel_panic(const char *message) {
    __asm__ volatile ("cli");

    serial_write("\n=== KERNEL PANIC ===\n");

    if (message != 0) {
        serial_write(message);
    } else {
        serial_write("Mensagem de panic ausente");
    }

    serial_write("\n");

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

    serial_write("\n=== KERNEL PANIC ===\n");

    if (message != 0) {
        serial_write(message);
    } else {
        serial_write("Mensagem de panic ausente");
    }

    serial_write("\nEndereco: ");
    serial_write_hex(address);
    serial_write("\n");

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