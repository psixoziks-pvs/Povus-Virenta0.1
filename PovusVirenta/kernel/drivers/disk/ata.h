/* ============================================================
 * Povus:Virenta — ATA (PIO) драйвер диска
 * Primary channel, disk0. 28-bit LBA.
 * ============================================================ */
#ifndef KERNEL_DRIVERS_ATA_H
#define KERNEL_DRIVERS_ATA_H

#include <stdbool.h>
#include <stdint.h>

bool ata_init(void);
bool ata_read(uint32_t lba, uint32_t count, void *buf);
bool ata_write(uint32_t lba, uint32_t count, const void *buf);
uint32_t ata_sectors(void);
const char *ata_model(void);

#endif
