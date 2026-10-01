/* ============================================================
 * Povus:Virenta — isr.h
 * Обработчики исключений CPU и диспетчер IRQ.
 * ============================================================ */
#ifndef KERNEL_ARCH_ISR_H
#define KERNEL_ARCH_ISR_H

#include "idt.h"

void isr_register_exception_handlers(void);
void panic(const char *msg, registers_t *regs);

#endif
