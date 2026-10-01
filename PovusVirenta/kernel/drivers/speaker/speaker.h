/* ============================================================
 * Povus:Virenta — PC Speaker (мягкие «природные» звуки)
 * ============================================================ */
#ifndef KERNEL_DRIVERS_SPEAKER_H
#define KERNEL_DRIVERS_SPEAKER_H

void speaker_beep(int freq_hz, int ms);
/* Фирменные звуки темы Virenta */
void sound_startup(void);      /* мягкий блип + восходящая трель */
void sound_notify(void);       /* капля в воду: короткий свист вверх */
void sound_error(void);        /* не пугающий низкий двойной тон */
void sound_click(void);        /* тихий клик */

#endif
