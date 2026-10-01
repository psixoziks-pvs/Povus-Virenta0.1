/* ============================================================
 * Povus:Virenta — PCI config space (mechanism #1, ports 0xCF8/0xCFC)
 * Нужен для поиска видеокарты (VBE/LFB) и будущих драйверов.
 * ============================================================ */
#ifndef KERNEL_DRIVERS_PCI_H
#define KERNEL_DRIVERS_PCI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct { uint16_t bus, dev, func; uint16_t vendor, device, classcode; } pci_dev_t;

void     pci_init(void);
uint32_t pci_read32(uint16_t bus, uint16_t dev, uint16_t func, uint16_t off);
void     pci_write32(uint16_t bus, uint16_t dev, uint16_t func, uint16_t off, uint32_t val);
bool     pci_find(uint16_t vendor, uint16_t device, pci_dev_t *out);
int      pci_enumerate(pci_dev_t *list, int max);
void     pci_dump(void);

#endif
