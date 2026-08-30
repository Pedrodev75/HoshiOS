#ifndef SERIAL_H
#define SERIAL_H

#include "utypes.h"

int serial_init(void);
int serial_is_ready(void);
int serial_putchar(char character);
void serial_write(const char *text);
void serial_write_hex(uint32_t value);

#endif