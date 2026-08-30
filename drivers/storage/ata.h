#ifndef ATA_H
#define ATA_H

#include "utypes.h"

/* Detecta o disco mestre no canal ATA primario e le sua capacidade LBA28. */
int ata_init(void);
int ata_is_available(void);
uint32_t ata_get_sector_count(void);

/* Le ou escreve exatamente um setor de 512 bytes em modo PIO/LBA28.
 * Retorna 0 em sucesso e -1 em falha. */
int ata_read_sector(uint32_t lba, uint8_t *buffer);
int ata_write_sector(uint32_t lba, const uint8_t *buffer);

#endif
