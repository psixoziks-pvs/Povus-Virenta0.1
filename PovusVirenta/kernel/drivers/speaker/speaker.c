/* ============================================================
 * Povus:Virenta — speaker.c
 * Звук через PIT-канал 2 + порт динамика (bit0, bit1 порта 0x61).
 * ============================================================ */
#include "speaker.h"
#include "drivers/timer/pit.h"
#include "arch/x86_64/io.h"

static void speaker_on(int freq) {
    if (freq <= 0) return;
    uint32_t div = 1193182 / (uint32_t)freq;
    outb(0x43, 0xB6);            /* channel 2 */
    outb(0x42, div & 0xFF);
    outb(0x42, (div >> 8) & 0xFF);
    uint8_t s = inb(0x61);
    outb(0x61, (s & ~3) | 3);    /* включить канал + gate */
}
static void speaker_off(void) {
    uint8_t s = inb(0x61);
    outb(0x61, s & ~3);
}

void speaker_beep(int freq_hz, int ms) {
    speaker_on(freq_hz);
    sleep_ms(ms);
    speaker_off();
    sleep_ms(15);                /* короткая пауза между тонами */
}

void sound_startup(void) {
    /* «блип + пение птиц»: три восходящих коротких тона */
    speaker_beep(523, 90);       /* C5 */
    speaker_beep(659, 90);       /* E5 */
    speaker_beep(784, 140);      /* G5 */
}
void sound_notify(void) {
    speaker_beep(880, 60);       /* A5 — капля */
    speaker_beep(1175, 70);      /* D6 — всплеск */
}
void sound_error(void) {
    speaker_beep(220, 160);      /* A3 — мягко, не резко */
    speaker_beep(196, 200);      /* G3 */
}
void sound_click(void) {
    speaker_beep(1000, 18);
}
