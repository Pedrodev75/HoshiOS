#ifndef TIMER_H
#define TIMER_H

#include "utypes.h"

void pit_init(uint32_t frequency);
void timer_tick(void);
uint32_t timer_get_ticks(void);
void timer_irq(void);

#endif