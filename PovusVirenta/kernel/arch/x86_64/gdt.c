/* ============================================================
 * Povus:Virenta — gdt.c
 * GDT + TSS для x86_64. TSS нужен для возврата из прерываний
 * пользовательских процессов (ist1 = стек ядра).
 * ============================================================ */
#include "gdt.h"
#include "io.h"
#include "lib/string.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags;      /* granularity | limit_high */
    uint8_t  base_high;
} __attribute__((packed));

/* Запись TSS в 64-битном режиме (42 байта + резерв) */
struct tss {
    uint32_t reserved0;
    uint64_t rsp[3];        /* index 0,1,2 по привилегии */
    uint64_t reserved1;
    uint64_t ist[7];        /* interrupt stacks, ist1 используем */
    uint64_t reserved2;
    uint32_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed));

/* В 64-битном режиме запись TSS занимает 16 байт (две записи GDT). */
static struct gdt_entry gdt_entries[8];

static struct tss tss;

static void set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_entries[i].base_low    = base & 0xFFFF;
    gdt_entries[i].base_mid    = (base >> 16) & 0xFF;
    gdt_entries[i].base_high   = (base >> 24) & 0xFF;
    gdt_entries[i].limit_low   = limit & 0xFFFF;
    gdt_entries[i].flags       = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt_entries[i].access      = access;
}

/* Длинная запись TSS: base 24..31, 32..63 */
static void set_tss_entry(int i, uint64_t base, uint32_t limit) {
    gdt_entries[i].limit_low   = limit & 0xFFFF;
    gdt_entries[i].base_low    = base & 0xFFFF;
    gdt_entries[i].base_mid    = (base >> 16) & 0xFF;
    gdt_entries[i].access      = 0x89;          /* TSS 64-bit, present */
    gdt_entries[i].flags       = ((limit >> 16) & 0x0F);
    gdt_entries[i].base_high   = (base >> 24) & 0xFF;
    /* Старшие 32 бита адреса — сразу после записи */
    *(uint32_t *)&gdt_entries[i + 1] = (uint32_t)(base >> 32);
}

extern void load_gdt(uint64_t gdtr_linear_addr);
extern void load_tss(uint16_t sel);

void gdt_init(void) {
    memset(gdt_entries, 0, sizeof(gdt_entries));
    memset(&tss, 0, sizeof(tss));

    set_entry(0, 0, 0, 0, 0);                              /* null */
    set_entry(1, 0, 0, 0x9A, 0x20);                        /* kernel code, L=1 */
    set_entry(2, 0, 0, 0x92, 0x20);                        /* kernel data */
    set_entry(3, 0, 0, 0xF2, 0x20);                        /* user data */
    set_entry(4, 0, 0, 0xFA, 0x20);                        /* user code */
    /* индекс 5 не используется отдельно (см. селекторы выше),
       но для совместимости с селекторами кладём TSS на индекс 6 */
    set_entry(5, 0, 0, 0xFA, 0x20);                        /* user code alias */

    /* TSS: стек ядра для прерываний ring3 -> ring0 */
    extern char kernel_stack_top[];
    tss.ist[0] = (uint64_t)kernel_stack_top;
    tss.rsp[0] = 0;
    tss.io_map_base = sizeof(tss);
    set_tss_entry(6, (uint64_t)&tss, sizeof(tss) - 1);

    struct gdtr { uint16_t len; uint64_t addr; } __attribute__((packed)) ptr =
        { sizeof(gdt_entries) - 1, (uint64_t)gdt_entries };

    load_gdt((uint64_t)&ptr);
    load_tss(GDT_TSS);
}
