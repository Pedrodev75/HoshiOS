#ifndef VGA_H
#define VGA_H
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#include "utypes.h"

enum vga_color {
    VGA_BLACK = 0x0,
    VGA_BLUE = 0x1,
    VGA_GREEN = 0x2,
    VGA_CYAN = 0x3,
    VGA_RED = 0x4,
    VGA_MAGENTA = 0x5,
    VGA_BROWN = 0x6,
    VGA_LIGHT_GREY = 0x7,
    VGA_DARK_GREY = 0x8,
    VGA_LIGHT_BLUE = 0x9,
    VGA_LIGHT_GREEN = 0xA,
    VGA_LIGHT_CYAN = 0xB,
    VGA_LIGHT_RED = 0xC,
    VGA_LIGHT_MAGENTA = 0xD,
    VGA_YELLOW = 0xE,
    VGA_WHITE = 0xF,
};

void vga_set_color(uint8_t fg, uint8_t bg);
void vga_putchar(char c);
void vga_print(const char* s);
void vga_clear(void);
void vga_backspace(void);
void vga_newline(uint16_t lines);
void vga_cursor_left(void);
void vga_cursor_right(void);
uint16_t vga_get_cursor(void);
void vga_set_cursor(uint16_t position);
void vga_print_uint(uint32_t value);
void vga_print_hex(uint32_t value);
uint32_t vga_get_scroll_count(void);

#endif
