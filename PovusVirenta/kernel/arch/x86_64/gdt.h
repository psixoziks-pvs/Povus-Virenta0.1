/* ============================================================
 * Povus:Virenta — GDT (Global Descriptor Table)
 * Кольца защиты: 0 = ядро, 3 = приложения.
 * ============================================================ */
#ifndef KERNEL_ARCH_GDT_H
#define KERNEL_ARCH_GDT_H

void gdt_init(void);

/* Селекторы сегментов */
#define GDT_KERNEL_CODE  0x08
#define GDT_KERNEL_DATA  0x10
#define GDT_USER_DATA    0x23   /* index 4 | RPL3 */
#define GDT_USER_CODE    0x2B   /* index 5 | RPL3 */
#define GDT_TSS          0x30   /* index 6 */

#endif
