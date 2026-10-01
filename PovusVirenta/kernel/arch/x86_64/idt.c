/* ============================================================
 * Povus:Virenta — idt.c
 * IDT + переназначение PIC (8259). ISR-стабы объявлены в isr.c.
 * ============================================================ */
#include "idt.h"
#include "io.h"
#include "lib/string.h"
#include "gdt.h"
#define GDT_KERNEL_CODE_VAL GDT_KERNEL_CODE

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  ist;        /* bits 0-2: index of interrupt stack */
    uint8_t  type_attr;  /* type | DPL<<5 | P<<7 */
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

static struct idt_entry idt[256] __attribute__((aligned(16)));

extern void isr_stub_table[];   /* массив из 256 адресов стабов (см. isr.c) */

void idt_init(void) {
    memset(idt, 0, sizeof(idt));
    for (int i = 0; i < 256; i++) {
        uint64_t base = (uint64_t)(uintptr_t)&isr_stub_table[i * 16];
        idt[i].base_low    = base & 0xFFFF;
        idt[i].selector    = GDT_KERNEL_CODE_VAL;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0x8E;          /* present, ring 0, 64-bit int gate */
        idt[i].base_mid    = (base >> 16) & 0xFFFF;
        idt[i].base_high   = (base >> 32) & 0xFFFFFFFF;
        idt[i].base_upper  = (base >> 64) & 0xFFFFFFFF;
    }
    struct idt_ptr ptr = { sizeof(idt) - 1, (uint64_t)&idt };
    __asm__ volatile ("lidt %0" :: "m"(ptr));
}
