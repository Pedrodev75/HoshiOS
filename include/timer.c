#include "timer.h"
#include "io.h"

#define PIT_FREQUENCY 1193182

static uint32_t timer_ticks = 0;

void pit_init(uint32_t frequency) {
    if (frequency == 0) {
        return;
    }

    uint16_t divisor = PIT_FREQUENCY / frequency;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
}

void timer_tick(void) {
    timer_ticks++;
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}