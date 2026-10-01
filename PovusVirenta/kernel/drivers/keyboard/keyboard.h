/* ============================================================
 * Povus:Virenta — клавиатура PS/2 (IRQ1, scancode set 1)
 * ============================================================ */
#ifndef KERNEL_DRIVERS_KEYBOARD_H
#define KERNEL_DRIVERS_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>
#include "arch/x86_64/idt.h"

void keyboard_init(void);
/* Следующий нажатый символ (0 если нет). Консумирует буфер. */
char keyboard_getc(void);
/* true если символ доступен */
bool keyboard_key_available(void);
/* Отладочный дамп последних scancode (для команды `ktest`) */
void keyboard_debug(uint8_t out[16], int *n);

#endif
