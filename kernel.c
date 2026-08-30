#include "include/io.h"
#include "include/vga.h"
#include "include/idt.h"
#include "include/shell.h"
#include "include/keyboard.h"
#include "include/panic.h"
#include "include/timer.h"
#include "include/pic.h"
#include "include/mouse.h"

#define INPUT_BUFFER_SIZE 128

static char input_buffer[INPUT_BUFFER_SIZE];
static uint16_t input_length = 0;
static uint16_t input_cursor = 0;
static uint16_t input_start = 0;

static void redraw_input_line(void) {
    uint32_t scrolls_before = vga_get_scroll_count();

    vga_set_cursor(input_start);

    for (uint16_t i = 0; i < input_length; i++) {
        vga_putchar(input_buffer[i]);
    }

    /* Apaga o caractere que pode ter sobrado após uma remoção. */
    vga_putchar(' ');

    uint32_t scrolls =
        vga_get_scroll_count() - scrolls_before;

    /*
     * Cada scroll desloca o começo da entrada uma linha para cima.
     */
    while (scrolls > 0) {
        if (input_start >= VGA_WIDTH) {
            input_start -= VGA_WIDTH;
        } else {
            input_start = 0;
        }

        scrolls--;
    }

    vga_set_cursor(input_start + input_cursor);
}

void lau_main() {
    vga_clear();
    vga_print("Bem-vindo ao Hoshi OS!\nDigite \"help\" pra ver os comandos disponiveis");
    vga_newline(2);
    vga_print("HoshiOS> ");
    input_start = vga_get_cursor();

    pic_remap();

    clear_IDT();

    set_idt_entry(&ir0, (uint8_t)0x8E, (uint8_t)0);
    set_idt_entry(&ir6, (uint8_t)0x8E, (uint8_t)6);
    set_idt_entry(&ir13, (uint8_t)0x8E, (uint8_t)13);
    set_idt_entry(&ir14, (uint8_t)0x8E, (uint8_t)14);
    set_idt_entry(&ir8, (uint8_t)0x8E, (uint8_t)8);
    set_idt_entry(&timer_irq, 0x8E, 32);
    set_idt_entry(&mouse_irq, 0x8E, 44);
    
    load_IDT();

    mouse_init();

    pit_init(100);
    pic_enable_timer_and_mouse();

    __asm__ volatile ("sti");

    static int caps_lock = 0;
    static int shift_pressed = 0;
    static int extended_scancode = 0;
  
    while (1) {
        uint8_t status = inb(0x64);

        if ((status & 0x01) && !(status & 0x20)) {
            uint8_t scancode = inb(0x60);

            if (scancode == 0xE0) {
                extended_scancode = 1;
                continue;
            }

            if (extended_scancode) {
                extended_scancode = 0;

                if (scancode == 0x4B && input_cursor > 0) {
                    input_cursor--;
                    vga_cursor_left();
                } else if (scancode == 0x4D && input_cursor < input_length) {
                    input_cursor++;
                    vga_cursor_right();
                }

                continue;
            }

            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
            }

            if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = 0;
             }

            if (scancode < 128) {
                if (scancode == 0x0E) {
                    if (input_cursor > 0) {
                        for (uint16_t i = input_cursor - 1; i < input_length - 1; i++) {
                            input_buffer[i] = input_buffer[i + 1];
                        }

                        input_cursor--;
                        input_length--;
                        input_buffer[input_length] = '\0';

                        redraw_input_line();
                    }
                }

                if (scancode == 0x1C) {
                    input_buffer[input_length] = '\0';
                    vga_newline(1);

                    shell_execute(input_buffer);

                    input_length = 0;
                    input_cursor = 0;
                    input_buffer[0] = '\0';

                    vga_newline(1);
                    vga_print("HoshiOS> ");
                    input_start = vga_get_cursor();
                }

                if (scancode == 0x3A) {
                    caps_lock = !caps_lock;
                }

                char character = keyboard_translate(
                    scancode,
                    shift_pressed,
                    caps_lock
                );

                if (character != 0) {
                    if (input_length < INPUT_BUFFER_SIZE - 1) {
                        for (uint16_t i = input_length; i > input_cursor; i--) {
                            input_buffer[i] = input_buffer[i - 1];
                        }

                        input_buffer[input_cursor] = character;
                        input_length++;
                        input_cursor++;

                        redraw_input_line();
                    }
                }
            }
        }
    }
}