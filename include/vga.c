#include "vga.h"
#include "io.h"
#include "mouse.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static volatile uint16_t* const vga = (uint16_t*)0xB8000;
static uint8_t current_color = (VGA_WHITE | (VGA_BLACK << 4));

static uint16_t cursor;

static void vga_update_cursor(void);
static void vga_scroll(void);
static void vga_update_cursor(void);
static void vga_scroll(void);
static uint32_t vga_lock(void);
static void vga_unlock(uint32_t flags);
static uint32_t scroll_count = 0;

void vga_set_color(uint8_t fg, uint8_t bg) {
    current_color = fg | (bg << 4);
}

void vga_putchar(char c) {
    uint32_t flags = vga_lock();

    vga[cursor++] = (current_color << 8) | (uint8_t)c;

    if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
        vga_scroll();
    }

    vga_update_cursor();
    vga_unlock(flags);
}

void vga_print(const char* s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') {
            vga_newline(1);
        } else {
            vga_putchar(s[i]);
        }
    }
}

void vga_clear(void) {
    uint32_t flags = vga_lock();

    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (current_color << 8) | ' ';
    }

    cursor = 0;
    vga_update_cursor();

    vga_unlock(flags);
}

void vga_backspace(void) {
    uint32_t flags = vga_lock();

    if (cursor > 0) {
        cursor--;
        vga[cursor] = (current_color << 8) | ' ';
        vga_update_cursor();
    }
    vga_unlock(flags);
}

void vga_cursor_left(void) {
    if (cursor > 0) {
        cursor--;
        vga_update_cursor();
    }
}

void vga_cursor_right(void) {
    if (cursor < VGA_WIDTH * VGA_HEIGHT - 1) {
        cursor++;
        vga_update_cursor();
    }
}

void vga_newline(uint16_t lines) {
    uint32_t flags = vga_lock();

    for (uint16_t i = 0; i < lines; i++) {
        cursor = ((cursor / VGA_WIDTH) + 1) * VGA_WIDTH;

        if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
            vga_scroll();
        }
    }

    vga_update_cursor();
    vga_unlock(flags);
}

uint16_t vga_get_cursor(void) {
    return cursor;
}

void vga_set_cursor(uint16_t position) {
    if (position < VGA_WIDTH * VGA_HEIGHT) {
        cursor = position;
        vga_update_cursor();
    }
}

static void vga_update_cursor(void) {
    outb(0x3D4, 0x0F);
    outb(0x3D5, cursor & 0xFF);

    outb(0x3D4, 0x0E);
    outb(0x3D5, (cursor >> 8) & 0xFF);
}

static void vga_scroll(void) {
    volatile uint16_t *video = (volatile uint16_t *)0xB8000;

    for (uint16_t row = 1; row < VGA_HEIGHT; row++) {
        for (uint16_t col = 0; col < VGA_WIDTH; col++) {
            video[(row - 1) * VGA_WIDTH + col] =
                video[row * VGA_WIDTH + col];
        }
    }

    for (uint16_t col = 0; col < VGA_WIDTH; col++) {
        video[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            ' ' | (current_color << 8);
    }

    cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
    scroll_count++;

    vga_update_cursor();
}

void vga_print_uint(uint32_t value) {
    char buffer[10];
    uint16_t length = 0;

    if (value == 0) {
        vga_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[length] = (value % 10) + '0';
        length++;
        value /= 10;
    }

    while (length > 0) {
        length--;
        vga_putchar(buffer[length]);
    }
}

void vga_print_hex(uint32_t value) {
    char buffer[10];
    uint16_t length = 0;

    if (value == 0) {
        vga_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[length] = "0123456789ABCDEF"[value % 16];
        length++;
        value /= 16;
    }

    while (length > 0) {
        length--;
        vga_putchar(buffer[length]);
    }
}

static uint32_t vga_lock(void) {
    uint32_t flags;

    __asm__ volatile (
        "pushfl\n"
        "cli\n"
        "popl %0"
        : "=r"(flags)
        :
        : "memory"
    );

    mouse_hide_cursor();
    return flags;
}

static void vga_unlock(uint32_t flags) {
    mouse_show_cursor();

    /* Bit 9 de EFLAGS indica se as interrupções estavam habilitadas. */
    if (flags & (1u << 9)) {
        __asm__ volatile ("sti" ::: "memory");
    }
}

uint32_t vga_get_scroll_count(void) {
    return scroll_count;
}