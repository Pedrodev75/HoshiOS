#ifndef PANIC_H
#define PANIC_H

#include "utypes.h"

void kernel_panic(const char *message);
void kernel_panic_addr(const char *message, uint32_t address);

#endif