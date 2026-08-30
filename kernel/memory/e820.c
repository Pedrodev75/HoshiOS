#include "e820.h"

#define E820_COUNT_ADDRESS 0x7FF0u
#define E820_MAP_ADDRESS   0x80000u

uint16_t e820_get_entry_count(void) {
    volatile uint16_t *count =
        (volatile uint16_t *)E820_COUNT_ADDRESS;

    if (*count > E820_MAX_ENTRIES) {
        return 0;
    }

    return *count;
}

const volatile e820_entry_t *
e820_get_entries(void) {
    return (const volatile e820_entry_t *)
        E820_MAP_ADDRESS;
}