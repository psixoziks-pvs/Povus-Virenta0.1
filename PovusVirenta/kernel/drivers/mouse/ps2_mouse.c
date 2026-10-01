/* ============================================================
 * Povus:Virenta — ps2_mouse.c
 * Инициализация aux-порта и разбор пакетов мыши.
 * ============================================================ */
#include "ps2_mouse.h"
#include "arch/x86_64/io.h"
#include "arch/x86_64/irq.h"
#include "lib/string.h"
#include "drivers/screen/vga.h"

static volatile mouse_state_t state;
static volatile int pkt_idx;
static volatile uint8_t pkt[3];
static volatile bool present;

static int kbd_wait_output(void) { for (int i = 0; i < 200000; i++) if (inb(0x64) & 1) return 0; return -1; }
static void kbd_write_cmd(uint8_t c) { while (inb(0x64) & 2) ; outb(0x64, c); }
static void aux_write(uint8_t c) {
    kbd_write_cmd(0xD4);
    while (inb(0x64) & 2) ;
    outb(0x60, c);
}
static uint8_t aux_read_ack(void) {
    if (kbd_wait_output()) return 0xFF;
    return inb(0x60);
}

static void mouse_handler(registers_t *regs) {
    (void)regs;
    if (!(inb(0x64) & 0x20)) return;   /* данные не с aux */
    uint8_t b = inb(0x60);
    if (pkt_idx == 0) {
        if ((b & 0x08) == 0) return;   /* потеря синхронизации */
        pkt[0] = b; pkt_idx = 1; return;
    }
    if (pkt_idx == 1) { pkt[1] = b; pkt_idx = 2; return; }
    pkt[2] = b; pkt_idx = 0;
    int dx = pkt[1] - ((pkt[0] & 0x40) ? 256 : 0);
    int dy = pkt[2] - ((pkt[0] & 0x20) ? 256 : 0);
    state.left   = pkt[0] & 1;
    state.right  = pkt[0] & 2;
    state.middle = pkt[0] & 4;
    state.x += dx;
    state.y -= dy;
    if (state.x < 0) state.x = 0;
    if (state.y < 0) state.y = 0;
    if (state.x > 1023) state.x = 1023;
    if (state.y > 767)  state.y = 767;
}

void ps2_mouse_init(void) {
    memset((void *)&state, 0, sizeof(state));
    present = false;
    kbd_write_cmd(0xA8);                 /* включить aux-порт */
    aux_write(0xF4);                     /* включить поток данных */
    if (aux_read_ack() != 0xFA) { kprintf("mouse: no ack\n"); return; }
    irq_set_handler(12, mouse_handler);
    present = true;
    kprintf("mouse: PS/2 ready (IRQ12)\n");
}

void ps2_mouse_get(mouse_state_t *out) {
    cli();
    *out = *(const mouse_state_t *)&state;
    sti();
}
bool ps2_mouse_present(void) { return present; }
