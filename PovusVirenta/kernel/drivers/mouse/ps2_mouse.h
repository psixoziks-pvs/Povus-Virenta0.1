/* ============================================================
 * Povus:Virenta — мышь PS/2 (aux через контроллер 8042, IRQ12)
 * Протокол 3-кнопочный, пакет 3 байта.
 * ============================================================ */
#ifndef KERNEL_DRIVERS_PS2_MOUSE_H
#define KERNEL_DRIVERS_PS2_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct { int x, y; bool left, right, middle; } mouse_state_t;

void ps2_mouse_init(void);
void ps2_mouse_get(mouse_state_t *out);
bool ps2_mouse_present(void);

#endif
