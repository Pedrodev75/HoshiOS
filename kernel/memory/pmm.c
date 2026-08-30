#include "pmm.h"
#include "e820.h"

#define PMM_PAGE_SIZE       4096u
#define PMM_LOW_MEMORY_END  0x00100000u
#define PMM_RESERVED_FRAMES (PMM_LOW_MEMORY_END / PMM_PAGE_SIZE)
#define PMM_4GB_LIMIT       0x100000000ULL

extern char __kernel_start;
extern char __kernel_end;

static uint8_t *bitmap;
static uint32_t bitmap_size;
static uint32_t bitmap_first_frame;
static uint32_t bitmap_frame_count;

static uint32_t kernel_first_frame;
static uint32_t kernel_end_frame;

static uint32_t total_frames;
static uint32_t free_frames;
static uint32_t used_frames;
static int pmm_initialized;

static uint32_t align_up_page(uint32_t address) {
    return (address + PMM_PAGE_SIZE - 1u) &
        ~(PMM_PAGE_SIZE - 1u);
}

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

static int pmm_get_entry_frames(
    const volatile e820_entry_t *entry,
    uint32_t *start_frame,
    uint32_t *end_frame
) {
    if (entry->type != E820_TYPE_USABLE ||
        entry->length == 0) {
        return -1;
    }

    uint64_t base = entry->base;
    uint64_t end = base + entry->length;

    if (end < base || base >= PMM_4GB_LIMIT) {
        return -1;
    }

    if (end > PMM_4GB_LIMIT) {
        end = PMM_4GB_LIMIT;
    }

    uint64_t aligned_start =
        (base + PMM_PAGE_SIZE - 1u) &
        ~((uint64_t)PMM_PAGE_SIZE - 1u);

    uint64_t aligned_end =
        end & ~((uint64_t)PMM_PAGE_SIZE - 1u);

    if (aligned_start >= aligned_end) {
        return -1;
    }

    *start_frame = (uint32_t)(aligned_start >> 12);
    *end_frame = (uint32_t)(aligned_end >> 12);

    return 0;
}

static void pmm_reserve_frame(uint32_t frame) {
    if (frame >= total_frames) {
        return;
    }

    if (!pmm_is_used(frame)) {
        pmm_mark_used(frame);
        free_frames--;
        used_frames++;
    }
}

static int pmm_frame_is_e820_usable(uint32_t frame) {
    uint16_t count = e820_get_entry_count();

    const volatile e820_entry_t *entries =
        e820_get_entries();

    for (uint16_t i = 0; i < count; i++) {
        uint32_t start;
        uint32_t end;

        if (pmm_get_entry_frames(
                &entries[i], &start, &end) != 0) {
            continue;
        }

        if (frame >= start && frame < end) {
            return 1;
        }
    }

    return 0;
}

static int pmm_frame_is_protected(uint32_t frame) {
    if (frame < PMM_RESERVED_FRAMES) {
        return 1;
    }

    if (frame >= kernel_first_frame &&
        frame < kernel_end_frame) {
        return 1;
    }

    if (frame >= bitmap_first_frame &&
        frame < bitmap_first_frame +
            bitmap_frame_count) {
        return 1;
    }

    return 0;
}

int pmm_init(void) {
    uint16_t count = e820_get_entry_count();

    const volatile e820_entry_t *entries =
        e820_get_entries();

    pmm_initialized = 0;
    total_frames = 0;
    free_frames = 0;
    used_frames = 0;

    if (count == 0) {
        return -1;
    }

    /*
     * Descobre o final da maior região utilizável abaixo de 4 GiB.
     */
    for (uint16_t i = 0; i < count; i++) {
        uint32_t start;
        uint32_t end;

        if (pmm_get_entry_frames(
                &entries[i], &start, &end) != 0) {
            continue;
        }

        if (end > total_frames) {
            total_frames = end;
        }
    }

    if (total_frames <= PMM_RESERVED_FRAMES) {
        return -1;
    }

    bitmap_size = (total_frames + 7u) / 8u;

    bitmap_frame_count =
        (bitmap_size + PMM_PAGE_SIZE - 1u) /
        PMM_PAGE_SIZE;

    kernel_first_frame =
        (uint32_t)&__kernel_start / PMM_PAGE_SIZE;

    kernel_end_frame =
        align_up_page((uint32_t)&__kernel_end) /
        PMM_PAGE_SIZE;

    uint32_t required_start = kernel_end_frame;

    if (required_start < PMM_RESERVED_FRAMES) {
        required_start = PMM_RESERVED_FRAMES;
    }

    /*
     * Procura uma região E820 utilizável onde o bitmap caiba.
     */
    bitmap_first_frame = 0;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t start;
        uint32_t end;

        if (pmm_get_entry_frames(
                &entries[i], &start, &end) != 0) {
            continue;
        }

        if (start < required_start) {
            start = required_start;
        }

        if (end > start &&
            end - start >= bitmap_frame_count) {
            bitmap_first_frame = start;
            break;
        }
    }

    if (bitmap_first_frame == 0) {
        return -1;
    }

    bitmap = (uint8_t *)(
        bitmap_first_frame * PMM_PAGE_SIZE
    );

    /*
     * Começa considerando todas as páginas ocupadas.
     */
    for (uint32_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    used_frames = total_frames;

    /*
     * Libera somente páginas pertencentes a regiões E820 do tipo 1.
     */
    for (uint16_t i = 0; i < count; i++) {
        uint32_t start;
        uint32_t end;

        if (pmm_get_entry_frames(
                &entries[i], &start, &end) != 0) {
            continue;
        }

        if (end > total_frames) {
            end = total_frames;
        }

        for (uint32_t frame = start;
             frame < end;
             frame++) {

            if (pmm_is_used(frame)) {
                pmm_mark_free(frame);
                free_frames++;
                used_frames--;
            }
        }
    }

    /*
     * Mantém o primeiro 1 MiB reservado.
     */
    for (uint32_t frame = 0;
         frame < PMM_RESERVED_FRAMES &&
         frame < total_frames;
         frame++) {

        pmm_reserve_frame(frame);
    }

    /*
     * Reserva a memória ocupada pelo kernel.
     */
    for (uint32_t frame = kernel_first_frame;
         frame < kernel_end_frame;
         frame++) {

        pmm_reserve_frame(frame);
    }

    /*
     * Reserva as páginas ocupadas pelo próprio bitmap.
     */
    for (uint32_t frame = bitmap_first_frame;
         frame < bitmap_first_frame +
             bitmap_frame_count;
         frame++) {

        pmm_reserve_frame(frame);
    }

    pmm_initialized = 1;
    return 0;
}

uint32_t pmm_alloc_frame(void) {
    if (!pmm_initialized || free_frames == 0) {
        return 0;
    }

    for (uint32_t frame = 0;
         frame < total_frames;
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
    if (!pmm_initialized ||
        address == 0 ||
        address % PMM_PAGE_SIZE != 0) {
        return -1;
    }

    uint32_t frame = address / PMM_PAGE_SIZE;

    if (frame >= total_frames ||
        !pmm_frame_is_e820_usable(frame) ||
        pmm_frame_is_protected(frame) ||
        !pmm_is_used(frame)) {
        return -1;
    }

    pmm_mark_free(frame);
    free_frames++;
    used_frames--;

    return 0;
}

uint32_t pmm_get_total_frames(void) {
    return total_frames;
}

uint32_t pmm_get_free_frames(void) {
    return free_frames;
}

uint32_t pmm_get_used_frames(void) {
    return used_frames;
}