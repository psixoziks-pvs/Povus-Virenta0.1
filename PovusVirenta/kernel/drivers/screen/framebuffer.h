/* ============================================================
 * Povus:Virenta — Framebuffer (LFB) + Bochs VBE dispi
 * Собственная графика без BIOS/VBE-вызовов из long mode:
 * порты 0x1CE/0x1CF работают на QEMU std-vga и bochs.
 * Пиксель: XRGB32 (little-endian).
 * ============================================================ */
#ifndef KERNEL_DRIVERS_FRAMEBUFFER_H
#define KERNEL_DRIVERS_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct { uint8_t r, g, b; } rgb_t;

bool     fb_init(void);                 /* true если графика доступна */
bool     fb_active(void);
uint32_t fb_width(void);
uint32_t fb_height(void);
void     fb_putpixel(uint32_t x, uint32_t y, rgb_t c);
rgb_t    fb_getpixel(uint32_t x, uint32_t y);
void     fb_clear(rgb_t c);
void     fb_rect(int x, int y, int w, int h, rgb_t c);
void     fb_fill_rounded(int x, int y, int w, int h, int r, rgb_t c);
void     fb_hgradient(int x, int y, int w, int h, rgb_t left, rgb_t right);
void     fb_vgradient_alpha(int x, int y, int w, int h, rgb_t top, rgb_t bottom,
                            uint8_t alpha_top, uint8_t alpha_bottom);
void     fb_blend_pixel(uint32_t x, uint32_t y, rgb_t c, uint8_t alpha);
void     fb_circle(int cx, int cy, int r, rgb_t c);
void     fb_disk(int cx, int cy, int r, rgb_t c);
void     fb_line(int x0, int y0, int x1, int y1, rgb_t c);
/* Blit RGB из внешнего буфера (для композитора окон) */
void     fb_blit_rgb(int x, int y, int w, int h, const uint8_t *src, int src_pitch);
/* Blit с альфа-каналом (RGBA source) */
void     fb_blit_rgba(int x, int y, int w, int h, const uint8_t *src, int src_pitch);
/* Текст 8x16 bitmap-шрифтом (встроенный) */
void     fb_draw_text(int x, int y, const char *s, rgb_t fg, rgb_t bg, bool bg_opaque);
int      fb_text_width(const char *s);

#endif
