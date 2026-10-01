/* ============================================================
 * Povus:Virenta — pci.c
 * Перечисление шин 0..255 (для образов QEMU хватает 0).
 * ============================================================ */
#include "pci.h"
#include "arch/x86_64/io.h"
#include "drivers/screen/vga.h"

static bool enabled;

void pci_init(void) { enabled = true; }

static uint32_t cfg_addr(uint16_t bus, uint16_t dev, uint16_t func, uint16_t off) {
    return (1u << 31) | (bus << 16) | (dev << 11) | (func << 8) | (off & 0xFC);
}

uint32_t pci_read32(uint16_t bus, uint16_t dev, uint16_t func, uint16_t off) {
    outl(0xCF8, cfg_addr(bus, dev, func, off));
    return inl(0xCFC);
}

void pci_write32(uint16_t bus, uint16_t dev, uint16_t func, uint16_t off, uint32_t val) {
    outl(0xCF8, cfg_addr(bus, dev, func, off));
    outl(0xCFC, val);
}

bool pci_find(uint16_t vendor, uint16_t device, pci_dev_t *out) {
    if (!enabled) return false;
    for (uint16_t bus = 0; bus < 1; bus++)
        for (uint16_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read32(bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0xFFFF) continue;
            if ((id & 0xFFFF) == vendor && ((id >> 16) & 0xFFFF) == device) {
                uint32_t cls = pci_read32(bus, dev, 0, 8);
                out->bus = bus; out->dev = dev; out->func = 0;
                out->vendor = vendor; out->device = device;
                out->classcode = (cls >> 16) & 0xFFFF;
                return true;
            }
        }
    return false;
}

int pci_enumerate(pci_dev_t *list, int max) {
    int n = 0;
    if (!enabled) return 0;
    for (uint16_t dev = 0; dev < 32 && n < max; dev++) {
        uint32_t id = pci_read32(0, dev, 0, 0);
        if ((id & 0xFFFF) == 0xFFFF || id == 0) continue;
        uint32_t cls = pci_read32(0, dev, 0, 8);
        list[n].bus = 0; list[n].dev = dev; list[n].func = 0;
        list[n].vendor = id & 0xFFFF;
        list[n].device = (id >> 16) & 0xFFFF;
        list[n].classcode = (cls >> 16) & 0xFFFF;
        n++;
    }
    return n;
}

void pci_dump(void) {
    pci_dev_t devs[16];
    int n = pci_enumerate(devs, 16);
    kprintf("PCI: %d devices\n", n);
    for (int i = 0; i < n; i++)
        kprintf("  %02x:%02x.%d  %x:%x  class %x\n",
                devs[i].bus, devs[i].dev, devs[i].func,
                devs[i].vendor, devs[i].device, devs[i].classcode);
}
