/* ============================================================
 * Povus:Virenta — pit.c
 * ============================================================ */
#include "pit.h"
#include "arch/x86_64/io.h"
#include "arch/x86_64/irq.h"

#define PIT_FREQ 1193182
static volatile uint64_t ticks;
static int tick_hz = 100;

static void pit_handler(registers_t *regs) {
    (void)regs;
    ticks++;
}

void pit_init(int hz) {
    if (hz <= 0) hz = 100;
    tick_hz = hz;
    ticks = 0;
    uint32_t div = PIT_FREQ / (uint32_t)hz;
    outb(0x43, 0x36);              /* channel 0, lo/hi, square wave */
    outb(0x40, div & 0xFF);
    outb(0x40, (div >> 8) & 0xFF);
    irq_set_handler(0, pit_handler);
}

uint64_t timer_ticks(void) { return ticks; }
uint64_t timer_ms(void)    { return ticks * (1000 / tick_hz); }

void sleep_ms(uint64_t ms) {
    uint64_t target = timer_ms() + ms;
    while (timer_ms() < target) hlt();
}
