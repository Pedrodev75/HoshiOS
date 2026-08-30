#ifndef PMM_H
#define PMM_H

#include "utypes.h"

int pmm_init(void);
uint32_t pmm_alloc_frame(void);
int pmm_free_frame(uint32_t address);

uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_free_frames(void);
uint32_t pmm_get_used_frames(void);

#endif