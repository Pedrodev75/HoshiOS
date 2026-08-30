#include "serial.h"
#include "io.h"

#define COM1_BASE       0x3F8
#define SERIAL_TIMEOUT  1000000u

#define SERIAL_DATA     (COM1_BASE + 0)
#define SERIAL_IER      (COM1_BASE + 1)
#define SERIAL_FIFO     (COM1_BASE + 2)
#define SERIAL_LCR      (COM1_BASE + 3)
#define SERIAL_MCR      (COM1_BASE + 4)
#define SERIAL_LSR      (COM1_BASE + 5)

#define SERIAL_LSR_TRANSMIT_EMPTY 0x20

static int serial_ready = 0;

static int serial_wait_transmit(void) {
    uint32_t timeout = SERIAL_TIMEOUT;

    while (timeout > 0) {
        if (inb(SERIAL_LSR) &
            SERIAL_LSR_TRANSMIT_EMPTY) {
            return 0;
        }

        timeout--;
    }

    return -1;
}

static int serial_putchar_raw(char character) {
    if (serial_wait_transmit() != 0) {
        return -1;
    }

    outb(SERIAL_DATA, (uint8_t)character);
    return 0;
}

int serial_init(void) {
    serial_ready = 0;

    /* Desabilita interrupções da UART. */
    outb(SERIAL_IER, 0x00);

    /*
     * Liga o DLAB para configurar a velocidade.
     * 115200 / 3 = 38400 baud.
     */
    outb(SERIAL_LCR, 0x80);
    outb(SERIAL_DATA, 0x03);
    outb(SERIAL_IER, 0x00);

    /* 8 bits, sem paridade e um stop bit. */
    outb(SERIAL_LCR, 0x03);

    /* Habilita e limpa os FIFOs. */
    outb(SERIAL_FIFO, 0xC7);

    /* Ativa o modo de teste interno. */
    outb(SERIAL_MCR, 0x1E);

    outb(SERIAL_DATA, 0xAE);

    if (inb(SERIAL_DATA) != 0xAE) {
        outb(SERIAL_MCR, 0x00);
        return -1;
    }

    /* Sai do teste e habilita a operação normal. */
    outb(SERIAL_MCR, 0x0F);

    serial_ready = 1;
    return 0;
}

int serial_is_ready(void) {
    return serial_ready;
}

int serial_putchar(char character) {
    if (!serial_ready) {
        return -1;
    }

    /*
     * Terminais seriais normalmente usam CR+LF para uma
     * quebra de linha completa.
     */
    if (character == '\n') {
        if (serial_putchar_raw('\r') != 0) {
            return -1;
        }
    }

    return serial_putchar_raw(character);
}

void serial_write(const char *text) {
    if (!serial_ready || text == 0) {
        return;
    }

    for (uint32_t i = 0; text[i] != '\0'; i++) {
        if (serial_putchar(text[i]) != 0) {
            return;
        }
    }
}

void serial_write_hex(uint32_t value) {
    static const char digits[] =
        "0123456789ABCDEF";

    serial_write("0x");

    for (int shift = 28; shift >= 0; shift -= 4) {
        uint8_t digit =
            (uint8_t)((value >> shift) & 0x0F);

        if (serial_putchar(digits[digit]) != 0) {
            return;
        }
    }
}