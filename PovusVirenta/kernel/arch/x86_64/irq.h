/* ============================================================
 * Povus:Virenta — API работы с IRQ и PIC
 * ============================================================ */
#ifndef KERNEL_ARCH_IRQ_H
#define KERNEL_ARCH_IRQ_H

#include "idt.h"

void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);

#endif
