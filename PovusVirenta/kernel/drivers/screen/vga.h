/* ============================================================
 * Povus:Virenta — VGA text mode (0xB8000), 80x25
 * Первый экран загрузки: лого, статусы инициализации, shell.
 * ============================================================ */
#ifndef KERNEL_DRIVERS_VGA_H
#define KERNEL_DRIVERS_VGA_H

#include <stdint.h>

/* Цвета EGA */
#define VGA_BLACK         0
#define VGA_BLUE          1
#define VGA_GREEN         2
#define VGA_CYAN          3
#define VGA_RED           4
#define VGA_MAGENTA       5
#define VGA_BROWN         6
#define VGA_LIGHT_GREY    7
#define VGA_DARK_GREY     8
#define VGA_LIGHT_BLUE    9
#define VGA_LIGHT_GREEN   10   /* ~ Virenta Green в палитре текста */
#define VGA_LIGHT_CYAN    11   /* ~ Aqua Blue */
#define VGA_LIGHT_RED     12
#define VGA_LIGHT_MAGENTA 13
#define VGA_YELLOW        14   /* ~ Solar Yellow */
#define VGA_WHITE         15

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char *s);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_get_color(uint8_t *fg, uint8_t *bg);
void vga_set_cursor(int x, int y);
void vga_scroll_up(int lines);
/* Вывод с фирменной темой: заголовок зелёным на тёмно-бирюзовом */
void vga_banner(const char *title, const char *subtitle);

/* Ядерный printf: пишет в VGA-текст и COM1-serial */
int  kprintf(const char *fmt, ...);
void kputc(char c);

#endif
