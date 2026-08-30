#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "utypes.h"

char keyboard_translate(
    uint8_t scancode,
    int shift_pressed,
    int caps_lock
);

#endif