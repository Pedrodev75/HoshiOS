#ifndef MOUSE_H
#define MOUSE_H

#include "utypes.h"

void mouse_init(void);
uint8_t mouse_read_byte(void);
void mouse_draw_cursor(int16_t x, int16_t y);
void mouse_hide_cursor(void);
void mouse_show_cursor(void);
void mouse_irq_handler(void);
void mouse_irq(void);

#endif