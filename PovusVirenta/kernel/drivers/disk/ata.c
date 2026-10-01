/* ============================================================
 * Povus:Virenta — ata.c
 * PIO-режим, по сектору за раз (просто и надёжно).
 * ============================================================ */
#include "ata.h"
#include "arch/x86_64/io.h"
#include "lib/string.h"

#define ATA_IO_BASE   0x1F0
#define ATA_CTRL      0x3F6
#define ATA_DATA      (ATA_IO_BASE + 0)
#define ATA_ERR       (ATA_IO_BASE + 1)
#define ATA_COUNT     (ATA_IO_BASE + 2)
#define ATA_LBA_LOW   (ATA_IO_BASE + 3)
#define ATA_LBA_MID   (ATA_IO_BASE + 4)
#define ATA_LBA_HI    (ATA_IO_BASE + 5)
#define ATA_DRIVE     (ATA_IO_BASE + 6)
#define ATA_CMD       (ATA_IO_BASE + 7)
#define ATA_STATUS    ATA_CMD

#define CMD_READ28   0x20
#define CMD_WRITE28  0x30
#define CMD_IDENTIFY 0xEC
#define ST_BSY  0x80
#define ST_DRQ  0x08
#define ST_ERR  0x01

static uint32_t sectors;
static char model[41];
static bool present;

static bool wait_drq(int allow_err) {
    for (int i = 0; i < 100000; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (s & ST_BSY) continue;
        if ((s & ST_DRQ) || (allow_err && (s & ST_ERR))) return true;
    }
    return false;
}

bool ata_init(void) {
    present = false;
    sectors = 0;
    strcpy(model, "unknown");

    outb(ATA_DRIVE, 0xA0);          /* master, LBA mode */
    io_wait(); io_wait(); io_wait(); io_wait();
    outb(ATA_COUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HI, 0);

    uint8_t st = inb(ATA_STATUS);
    if (st == 0 || st == 0xFF) return false;   /* диска нет */

    outb(ATA_CMD, CMD_IDENTIFY);
    io_wait();
    st = inb(ATA_STATUS);
    if (st == 0) return false;
    if (!wait_drq(0)) return false;

    uint16_t id[256];
    for (int i = 0; i < 256; i++) id[i] = inw(ATA_DATA);

    /* swap bytes в модели */
    for (int i = 0; i < 40; i += 2) {
        model[i]   = (char)(id[27 + i / 2] >> 8);
        model[i+1] = (char)(id[27 + i / 2] & 0xFF);
    }
    model[40] = 0;
    for (int i = 39; i >= 0 && model[i] == ' '; i--) model[i] = 0;

    sectors = ((uint32_t)id[61] << 16 | id[60]);
    if (sectors == 0) sectors = 1 << 22;   /* fallback ~2ГБ */
    present = true;
    return true;
}

uint32_t ata_sectors(void) { return sectors; }
const char *ata_model(void) { return model; }

bool ata_read(uint32_t lba, uint32_t count, void *buf) {
    if (!present) return false;
    uint16_t *w = buf;
    while (count--) {
        outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
        outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
        outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
        outb(ATA_LBA_LOW, lba & 0xFF);
        outb(ATA_COUNT, 1);
        outb(ATA_CMD, CMD_READ28);
        lba++;
        io_wait();
        if (!wait_drq(1)) return false;
        for (int i = 0; i < 256; i++) *w++ = inw(ATA_DATA);
    }
    return true;
}

bool ata_write(uint32_t lba, uint32_t count, const void *buf) {
    if (!present) return false;
    const uint16_t *r = buf;
    while (count--) {
        outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
        outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
        outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
        outb(ATA_LBA_LOW, lba & 0xFF);
        outb(ATA_COUNT, 1);
        outb(ATA_CMD, CMD_WRITE28);
        lba++;
        io_wait();
        if (!wait_drq(1)) return false;
        for (int i = 0; i < 256; i++) outw(ATA_DATA, *r++);
        if (!wait_drq(1)) return false;
    }
    outb(ATA_CMD, 0xE7);   /* flush cache */
    return true;
}
