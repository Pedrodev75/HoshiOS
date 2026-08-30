#include "utypes.h"

static const char scancode_to_char[128] = {
    [0x39] = ' ',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x1E] = 'a',
    [0x30] = 'b',
    [0x2E] = 'c',
    [0x20] = 'd',
    [0x12] = 'e',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x17] = 'i',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x32] = 'm',
    [0x31] = 'n',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x10] = 'q',
    [0x13] = 'r',
    [0x1F] = 's',
    [0x14] = 't',
    [0x16] = 'u',
    [0x2F] = 'v',
    [0x11] = 'w',
    [0x2D] = 'x',
    [0x15] = 'y',
    [0x2C] = 'z',
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
};

char keyboard_translate(
    uint8_t scancode,
    int shift_pressed,
    int caps_lock
) {
    if (scancode >= 128) {
        return 0;
    }

    char character = scancode_to_char[scancode];

    if (shift_pressed) {
        if (scancode == 0x02) character = '!';
        if (scancode == 0x03) character = '@';
        if (scancode == 0x04) character = '#';
        if (scancode == 0x05) character = '$';
        if (scancode == 0x06) character = '%';
        if (scancode == 0x08) character = '&';
        if (scancode == 0x09) character = '*';
        if (scancode == 0x0A) character = '(';
        if (scancode == 0x0B) character = ')';
    }

    if (caps_lock != shift_pressed &&
        character >= 'a' &&
        character <= 'z') {
        character = character - 'a' + 'A';
    }

    return character;
}