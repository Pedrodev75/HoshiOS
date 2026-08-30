#include "ata.h"
#include "io.h"
#include "vga.h"

/* ATA primario, disco mestre, PIO por polling e enderecamento LBA28. */

#define ATA_PRIMARY_DATA           0x1F0
#define ATA_PRIMARY_ERROR          0x1F1
#define ATA_PRIMARY_SECCOUNT       0x1F2
#define ATA_PRIMARY_LBA_LOW        0x1F3
#define ATA_PRIMARY_LBA_MID        0x1F4
#define ATA_PRIMARY_LBA_HIGH       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD     0x1F6
#define ATA_PRIMARY_STATUS         0x1F7
#define ATA_PRIMARY_COMMAND        0x1F7
#define ATA_PRIMARY_ALT_STATUS     0x3F6
#define ATA_PRIMARY_DEVICE_CONTROL 0x3F6

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF  0x20
#define ATA_STATUS_BSY 0x80

#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_CACHE_FLUSH   0xE7
#define ATA_CMD_IDENTIFY      0xEC

#define ATA_TIMEOUT 1000000u
#define ATA_LBA28_SECTOR_LIMIT 0x10000000u

static int ata_initialized = 0;
static int ata_available = 0;
static uint32_t ata_sector_count = 0;

static void ata_print_error(const char *message, uint8_t status) {
    vga_print(message);
    vga_print(" (status 0x");
    vga_print_hex(status);

    if (status & ATA_STATUS_ERR) {
        vga_print(", erro 0x");
        vga_print_hex(inb(ATA_PRIMARY_ERROR));
    }

    vga_print(")");
    vga_newline(1);
}

/* Quatro leituras da porta alternativa fornecem a espera de ~400 ns sem
 * limpar uma eventual IRQ pendente na porta de status principal. */
static void ata_delay_400ns(void) {
    inb(ATA_PRIMARY_ALT_STATUS);
    inb(ATA_PRIMARY_ALT_STATUS);
    inb(ATA_PRIMARY_ALT_STATUS);
    inb(ATA_PRIMARY_ALT_STATUS);
}

static int ata_wait_not_busy(uint8_t *last_status) {
    uint32_t timeout = ATA_TIMEOUT;

    while (timeout > 0) {
        uint8_t status = inb(ATA_PRIMARY_STATUS);

        if (status == 0x00 || status == 0xFF) {
            *last_status = status;
            return -1;
        }

        if (!(status & ATA_STATUS_BSY)) {
            *last_status = status;
            return 0;
        }

        timeout--;
    }

    *last_status = inb(ATA_PRIMARY_STATUS);
    return -1;
}

static int ata_wait_drq(uint8_t *last_status) {
    uint32_t timeout = ATA_TIMEOUT;

    while (timeout > 0) {
        uint8_t status = inb(ATA_PRIMARY_STATUS);

        if (status == 0x00 || status == 0xFF ||
            (status & (ATA_STATUS_ERR | ATA_STATUS_DF))) {
            *last_status = status;
            return -1;
        }

        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) {
            *last_status = status;
            return 0;
        }

        timeout--;
    }

    *last_status = inb(ATA_PRIMARY_STATUS);
    return -1;
}

static int ata_wait_complete(uint8_t *last_status) {
    uint32_t timeout = ATA_TIMEOUT;

    while (timeout > 0) {
        uint8_t status = inb(ATA_PRIMARY_STATUS);

        if (status == 0x00 || status == 0xFF ||
            (status & (ATA_STATUS_ERR | ATA_STATUS_DF))) {
            *last_status = status;
            return -1;
        }

        if (!(status & (ATA_STATUS_BSY | ATA_STATUS_DRQ))) {
            *last_status = status;
            return 0;
        }

        timeout--;
    }

    *last_status = inb(ATA_PRIMARY_STATUS);
    return -1;
}

static int ata_ensure_available(void) {
    if (!ata_initialized) {
        return ata_init();
    }

    return ata_available ? 0 : -1;
}

static int ata_validate_request(uint32_t lba, const void *buffer) {
    if (buffer == 0) {
        vga_print("ATA: buffer invalido");
        vga_newline(1);
        return -1;
    }

    if (lba >= ATA_LBA28_SECTOR_LIMIT ||
        (ata_sector_count != 0 && lba >= ata_sector_count)) {
        vga_print("ATA: LBA fora do disco");
        vga_newline(1);
        return -1;
    }

    return 0;
}

static void ata_select_and_setup(uint32_t lba) {
    outb(ATA_PRIMARY_DRIVE_HEAD,
         (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    ata_delay_400ns();

    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
}

int ata_init(void) {
    uint16_t identify[256];
    uint8_t status;

    ata_initialized = 1;
    ata_available = 0;
    ata_sector_count = 0;

    /* O driver usa polling; desabilita a IRQ do canal ATA primario. */
    outb(ATA_PRIMARY_DEVICE_CONTROL, 0x02);

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    ata_delay_400ns();

    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HIGH, 0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);

    status = inb(ATA_PRIMARY_STATUS);
    if (status == 0x00 || status == 0xFF) {
        vga_print("ATA: nenhum disco mestre detectado");
        vga_newline(1);
        return -1;
    }

    if (ata_wait_not_busy(&status) != 0) {
        ata_print_error("ATA: timeout durante IDENTIFY", status);
        return -1;
    }

    /* Valores diferentes de zero indicam, normalmente, um dispositivo
     * ATAPI (CD-ROM), que este driver nao implementa. */
    if (inb(ATA_PRIMARY_LBA_MID) != 0 ||
        inb(ATA_PRIMARY_LBA_HIGH) != 0) {
        vga_print("ATA: dispositivo detectado nao e ATA PIO");
        vga_newline(1);
        return -1;
    }

    if (ata_wait_drq(&status) != 0) {
        ata_print_error("ATA: IDENTIFY falhou", status);
        return -1;
    }

    for (uint16_t i = 0; i < 256; i++) {
        identify[i] = inw(ATA_PRIMARY_DATA);
    }

    if (!(identify[49] & (1u << 9))) {
        vga_print("ATA: disco nao suporta LBA");
        vga_newline(1);
        return -1;
    }

    ata_sector_count =
        (uint32_t)identify[60] |
        ((uint32_t)identify[61] << 16);

    if (ata_sector_count == 0) {
        vga_print("ATA: capacidade LBA invalida");
        vga_newline(1);
        return -1;
    }

    ata_available = 1;
    return 0;
}

int ata_is_available(void) {
    return ata_available;
}

uint32_t ata_get_sector_count(void) {
    return ata_sector_count;
}

int ata_read_sector(uint32_t lba, uint8_t *buffer) {
    uint8_t status;

    if (ata_ensure_available() != 0 ||
        ata_validate_request(lba, buffer) != 0) {
        return -1;
    }

    if (ata_wait_complete(&status) != 0) {
        ata_print_error("ATA: disco nao ficou pronto para leitura", status);
        return -1;
    }

    ata_select_and_setup(lba);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);

    if (ata_wait_drq(&status) != 0) {
        ata_print_error("ATA: erro ao ler setor", status);
        return -1;
    }

    for (uint16_t i = 0; i < 256; i++) {
        uint16_t word = inw(ATA_PRIMARY_DATA);
        buffer[i * 2] = (uint8_t)(word & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)(word >> 8);
    }

    if (ata_wait_complete(&status) != 0) {
        ata_print_error("ATA: leitura nao foi finalizada", status);
        return -1;
    }

    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t *buffer) {
    uint8_t status;

    if (ata_ensure_available() != 0 ||
        ata_validate_request(lba, buffer) != 0) {
        return -1;
    }

    if (ata_wait_complete(&status) != 0) {
        ata_print_error("ATA: disco nao ficou pronto para escrita", status);
        return -1;
    }

    ata_select_and_setup(lba);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);

    if (ata_wait_drq(&status) != 0) {
        ata_print_error("ATA: erro ao preparar escrita", status);
        return -1;
    }

    for (uint16_t i = 0; i < 256; i++) {
        uint16_t word =
            (uint16_t)buffer[i * 2] |
            ((uint16_t)buffer[i * 2 + 1] << 8);
        outw(ATA_PRIMARY_DATA, word);
    }

    if (ata_wait_complete(&status) != 0) {
        ata_print_error("ATA: escrita nao foi finalizada", status);
        return -1;
    }

    outb(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);

    if (ata_wait_complete(&status) != 0) {
        ata_print_error("ATA: cache flush falhou", status);
        return -1;
    }

    return 0;
}
