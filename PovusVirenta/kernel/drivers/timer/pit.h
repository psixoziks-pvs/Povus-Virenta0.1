/* ============================================================
 * Povus:Virenta — PIT 8253/8254 таймер (IRQ0, 100 Гц)
 * ============================================================ */
#ifndef KERNEL_DRIVERS_PIT_H
#define KERNEL_DRIVERS_PIT_H

#include <stdint.h>

void pit_init(int hz);
uint64_t timer_ticks(void);       /* тики с момента инициализации */
uint64_t timer_ms(void);          /* миллисекунды (hz=100 → tick=10ms) */
void sleep_ms(uint64_t ms);

#endif
