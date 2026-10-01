/* ============================================================
 * Povus:Virenta — Портный ввод/вывод (x86_64, ring 0)
 * Обёртки над in/out инструкциями.
 * ============================================================ */
#ifndef KERNEL_ARCH_IO_H
#define KERNEL_ARCH_IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Задержка ~1 мкс для медленных ISA-портов */
static inline void io_wait(void) { outb(0x80, 0); }

static inline void cli(void) { __asm__ volatile ("cli"); }
static inline void sti(void) { __asm__ volatile ("sti"); }
static inline void hlt(void) { __asm__ volatile ("hlt"); }

/* Выход из QEMU (isa-debug-exit, base 0x500). Код >>1 читается как status. */
static inline void qemu_exit(int code) { outw(0x501, (uint16_t)((code << 1) | 0x10)); }

/* Тёплая перезагрузка через контроллер клавиатуры (порт 0x64, команда 0xFE) */
static inline void reboot(void) {
    while (inb(0x64) & 1) inb(0x60);   /* очистить output buffer */
    outb(0x64, 0xFE);                  /* CPU reset */
    for (;;) hlt();
}

#endif /* KERNEL_ARCH_IO_H */
