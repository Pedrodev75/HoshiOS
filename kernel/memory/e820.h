#ifndef E820_H
#define E820_H

#include "utypes.h"

#define E820_TYPE_USABLE 1u
#define E820_MAX_ENTRIES 128u

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t attributes;
} e820_entry_t;

uint16_t e820_get_entry_count(void);

const volatile e820_entry_t *
e820_get_entries(void);

#endif