#include "mouse.h"
#include "io.h"
#include "vga.h"
#include "timer.h"

static uint16_t old_position;
static uint16_t old_cell;
static int cursor_visible = 0;
static uint8_t mouse_packet[3];
static uint8_t mouse_cycle = 0;
static int16_t mouse_x = 40;
static int16_t mouse_y = 12;
static uint32_t mouse_packet_start_tick = 0;
static int mouse_ready = 0;

/* O mouse reporta deltas em contagens bem mais finas do que uma celula
 * de texto (80x25). Sem isso, um movimento minimo no mouse real ja
 * pula varias colunas/linhas de uma vez, parecendo erratico. Acumula
 * o delta bruto e so avanca o cursor a cada MOUSE_SENSITIVITY_DIVISOR
 * unidades, guardando o resto fracionario pra nao perder precisao. */
#define MOUSE_SENSITIVITY_DIVISOR 10
#define MOUSE_IO_TIMEOUT 1000000u
static int16_t accum_x = 0;
static int16_t accum_y = 0;

/* Descarta qualquer byte que ja esteja parado no buffer de saida do
 * controlador 8042 (teclado ou mouse) antes de comecar a sequencia de
 * inicializacao. Sem isso, um byte de teclado deixado no buffer (ex:
 * usuario apertou uma tecla durante o boot) pode ser lido por engano
 * como se fosse o ACK/self-test/ID do mouse. */
static void mouse_flush_buffer(void) {
    uint16_t guard = 0;

    while ((inb(0x64) & 0x01) && guard < 64) {
        inb(0x60);
        guard++;
    }
}

static int mouse_wait_write(void) {
    uint32_t timeout = MOUSE_IO_TIMEOUT;

    while (timeout > 0) {
        if (!(inb(0x64) & 0x02)) {
            return 0;
        }

        timeout--;
    }

    return -1;
}

static int mouse_wait_read(void) {
    uint32_t timeout = MOUSE_IO_TIMEOUT;

    while (timeout > 0) {
        if (inb(0x64) & 0x01) {
            return 0;
        }

        timeout--;
    }

    return -1;
}

static int mouse_send_command(uint8_t command, uint8_t *response) {
    if (mouse_wait_write() != 0) {
        return -1;
    }

    outb(0x64, 0xD4);

    if (mouse_wait_write() != 0) {
        return -1;
    }

    outb(0x60, command);

    if (mouse_wait_read() != 0) {
        return -1;
    }

    *response = inb(0x60);
    return 0;
}

void mouse_init(void) {
    uint8_t command_byte;
    uint8_t response;

    mouse_ready = 0;
    mouse_flush_buffer();

    /* Habilita a porta auxiliar PS/2. */
    if (mouse_wait_write() != 0) {
        vga_print("Mouse: timeout ao habilitar porta");
        vga_newline(1);
        return;
    }

    outb(0x64, 0xA8);

    /* Solicita o byte de configuracao do controlador. */
    if (mouse_wait_write() != 0) {
        vga_print("Mouse: timeout no controlador");
        vga_newline(1);
        return;
    }

    outb(0x64, 0x20);

    if (mouse_wait_read() != 0) {
        vga_print("Mouse: timeout lendo configuracao");
        vga_newline(1);
        return;
    }

    command_byte = inb(0x60);

    /* Habilita IRQ12 e habilita o clock da porta auxiliar. */
    command_byte &= (uint8_t)~0x20;
    command_byte |= 0x02;

    if (mouse_wait_write() != 0) {
        vga_print("Mouse: timeout preparando configuracao");
        vga_newline(1);
        return;
    }

    outb(0x64, 0x60);

    if (mouse_wait_write() != 0) {
        vga_print("Mouse: timeout gravando configuracao");
        vga_newline(1);
        return;
    }

    outb(0x60, command_byte);

    /* Reinicia o mouse. */
    if (mouse_send_command(0xFF, &response) != 0) {
        vga_print("Mouse: timeout durante reset");
        vga_newline(1);
        return;
    }

    if (response != 0xFA) {
        vga_print("Falha no reset do mouse");
        vga_newline(1);
        return;
    }

    /* Aguarda o resultado do autoteste. */
    if (mouse_wait_read() != 0) {
        vga_print("Mouse: timeout no autoteste");
        vga_newline(1);
        return;
    }

    if (inb(0x60) != 0xAA) {
        vga_print("Mouse falhou no autoteste");
        vga_newline(1);
        return;
    }

    /* Lê e descarta o ID do mouse. */
    if (mouse_wait_read() != 0) {
        vga_print("Mouse: timeout lendo ID");
        vga_newline(1);
        return;
    }

    inb(0x60);

    /* Restaura as configuracoes padrao. */
    if (mouse_send_command(0xF6, &response) != 0 ||
        response != 0xFA) {
        vga_print("Falha ao configurar mouse");
        vga_newline(1);
        return;
    }

    /* Ativa o envio de pacotes. */
    if (mouse_send_command(0xF4, &response) != 0 ||
        response != 0xFA) {
        vga_print("Falha ao ativar streaming");
        vga_newline(1);
        return;
    }

    mouse_ready = 1;
    mouse_show_cursor();
}

uint8_t mouse_read_byte(void) {
    uint8_t status = inb(0x64);

    if ((status & 0x21) != 0x21) {
        return 0;
    }

    return inb(0x60);
}

void mouse_draw_cursor(int16_t x, int16_t y) {
    volatile uint16_t *video =
        (volatile uint16_t *)0xB8000;

    if (x < 0 || x >= VGA_WIDTH ||
        y < 0 || y >= VGA_HEIGHT) {
        return;
    }

    if (cursor_visible) {
        video[old_position] = old_cell;
    }

    uint16_t position =
        (uint16_t)(y * VGA_WIDTH + x);

    old_position = position;
    old_cell = video[position];

    video[position] =
        (uint16_t)'X' |
        ((uint16_t)VGA_LIGHT_RED << 8);

    cursor_visible = 1;
}

/* Restaura o conteudo original por baixo do cursor, sem redesenhar.
 * Precisa ser chamada ANTES de qualquer coisa que mova/reescreva a
 * tela por fora do mouse.c (como um scroll), senao o glifo do 'X'
 * fica gravado no meio do conteudo deslocado e vira um cursor
 * "fantasma" que nao responde mais a movimento nenhum. */
void mouse_hide_cursor(void) {
    if (cursor_visible) {
        volatile uint16_t *video =
            (volatile uint16_t *)0xB8000;

        video[old_position] = old_cell;
        cursor_visible = 0;
    }
}

/* Redesenha o cursor na posicao atual (mouse_x/mouse_y). Chamar depois
 * de qualquer operacao que tenha alterado a tela por fora do mouse.c,
 * pra recolocar o 'X' por cima do conteudo novo. */
void mouse_show_cursor(void) {
    if (!mouse_ready) {
        return;
    }

    mouse_draw_cursor(mouse_x, mouse_y);
}

void mouse_irq_handler(void) {
    uint8_t data = inb(0x60);

    if (!mouse_ready) {
        return;
    }

    /* Se estamos no meio de um pacote e ja se passou tempo demais desde
     * o primeiro byte (ex: uma IRQ foi perdida), o pacote atual esta
     * corrompido: descarta e trata este byte como um novo inicio. Sem
     * isso, uma unica IRQ perdida desalinha os bytes permanentemente. */
    if (mouse_cycle != 0 &&
        (timer_get_ticks() - mouse_packet_start_tick) > 5) {
        mouse_cycle = 0;
    }

    if (mouse_cycle == 0 && !(data & 0x08)) {
        return;
    }

    if (mouse_cycle == 0) {
        mouse_packet_start_tick = timer_get_ticks();
    }

    mouse_packet[mouse_cycle] = data;
    mouse_cycle++;

    if (mouse_cycle != 3) {
        return;
    }

    mouse_cycle = 0;

    int8_t delta_x = (int8_t)mouse_packet[1];
    int8_t delta_y = (int8_t)mouse_packet[2];

    accum_x += delta_x;
    accum_y += delta_y;

    int16_t move_x = accum_x / MOUSE_SENSITIVITY_DIVISOR;
    int16_t move_y = accum_y / MOUSE_SENSITIVITY_DIVISOR;

    accum_x -= move_x * MOUSE_SENSITIVITY_DIVISOR;
    accum_y -= move_y * MOUSE_SENSITIVITY_DIVISOR;

    mouse_x += move_x;
    mouse_y -= move_y;

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x >= VGA_WIDTH) mouse_x = VGA_WIDTH - 1;

    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y >= VGA_HEIGHT) mouse_y = VGA_HEIGHT - 1;

    mouse_draw_cursor(mouse_x, mouse_y);
}