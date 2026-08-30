#include "pmm.h"

#define PMM_PAGE_SIZE       4096u
#define PMM_MEMORY_SIZE     (32u * 1024u * 1024u)
#define PMM_TOTAL_FRAMES    (PMM_MEMORY_SIZE / PMM_PAGE_SIZE)
#define PMM_RESERVED_FRAMES (1u * 1024u * 1024u / PMM_PAGE_SIZE)
#define PMM_BITMAP_SIZE     ((PMM_TOTAL_FRAMES + 7u) / 8u)

static uint8_t bitmap[PMM_BITMAP_SIZE];
static uint32_t free_frames;
static uint32_t used_frames;

static void pmm_mark_used(uint32_t frame) {
    uint32_t byte_index = frame / 8u;
    uint8_t bit_index = (uint8_t)(frame % 8u);
    uint8_t mask = (uint8_t)(1u << bit_index);

    bitmap[byte_index] |= mask;
}

static void pmm_mark_free(uint32_t frame) {
    uint32_t byte_index = frame / 8u;
    uint8_t bit_index = (uint8_t)(frame % 8u);
    uint8_t mask = (uint8_t)(1u << bit_index);

    bitmap[byte_index] &= (uint8_t)~mask;
}

static int pmm_is_used(uint32_t frame) {
    uint32_t byte_index = frame / 8u;
    uint8_t bit_index = (uint8_t)(frame % 8u);
    uint8_t mask = (uint8_t)(1u << bit_index);

    return (bitmap[byte_index] & mask) != 0;
}

void pmm_init(void) {
    for (uint32_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        bitmap[i] = 0xFF;
    }

    for (uint32_t frame = PMM_RESERVED_FRAMES; frame < PMM_TOTAL_FRAMES; frame++) {

        pmm_mark_free(frame);
    }

    free_frames = PMM_TOTAL_FRAMES - PMM_RESERVED_FRAMES;
    used_frames = PMM_RESERVED_FRAMES;
}

uint32_t pmm_get_total_frames(void) {
    return PMM_TOTAL_FRAMES;
}

uint32_t pmm_get_free_frames(void) {
    return free_frames;
}

uint32_t pmm_get_used_frames(void) {
    return used_frames;
}

uint32_t pmm_alloc_frame(void) {
    if (free_frames == 0) {
        return 0;
    }

    for (uint32_t frame = PMM_RESERVED_FRAMES;
         frame < PMM_TOTAL_FRAMES;
         frame++) {

        if (!pmm_is_used(frame)) {
            pmm_mark_used(frame);

            free_frames--;
            used_frames++;

            return frame * PMM_PAGE_SIZE;
        }
    }

    return 0;
}

int pmm_free_frame(uint32_t address) {
    if (address == 0 ||
        address % PMM_PAGE_SIZE != 0) {
        return -1;
    }

    uint32_t frame = address / PMM_PAGE_SIZE;

    if (frame < PMM_RESERVED_FRAMES ||
        frame >= PMM_TOTAL_FRAMES) {
        return -1;
    }

    if (!pmm_is_used(frame)) {
        return -1;
    }

    pmm_mark_free(frame);

    free_frames++;
    used_frames--;

    return 0;
}