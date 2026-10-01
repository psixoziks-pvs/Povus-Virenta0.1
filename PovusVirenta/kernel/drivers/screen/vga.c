/* ============================================================
 * Povus:Virenta — vga.c
 * Текст-режим VGA + порт serial 0x3F8 для логов QEMU.
 * kprintf пишет одновременно на экран и в COM1.
 * ============================================================ */
#include "vga.h"
#include "arch/x86_64/io.h"
#include "lib/printf.h"
#include "lib/string.h"

#define VGA_MEMORY ((uint16_t *)0x0B8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Serial COM1 (для `make run` с -serial stdio) */
#define COM1 0x3F8
static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x80);
    outb(COM1 + 3, 0x07);
    outb(COM1 + 2, 0xC3);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 1, 0x01);
}
static void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0) ;
    outb(COM1, (uint8_t)c);
}

static size_t cursor_x = 0, cursor_y = 0;
static uint8_t colorattr = 0x0F;   /* light grey on black */

static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)(pos >> 8));
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    colorattr = (bg << 4) | (fg & 0x0F);
}
void vga_get_color(uint8_t *fg, uint8_t *bg) {
    if (fg) *fg = colorattr & 0x0F;
    if (bg) *bg = colorattr >> 4;
}

void vga_scroll_up(int lines) {
    if (lines > (int)cursor_y) lines = (int)cursor_y;
    for (int i = 0; i < (VGA_HEIGHT - lines) * VGA_WIDTH; i++)
        VGA_MEMORY[i] = VGA_MEMORY[lines * VGA_WIDTH + i];
    for (int i = (VGA_HEIGHT - lines) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
        VGA_MEMORY[i] = (colorattr << 8) | ' ';
    cursor_y -= lines;
}

void vga_putc(char c) {
    if (c == '\n' || c == '\r') {
        if (c == '\n') cursor_x = 0;
        if (++cursor_y >= VGA_HEIGHT) vga_scroll_up(1);
        update_cursor();
        return;
    }
    if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7u;
        if (cursor_x >= VGA_WIDTH) { cursor_x = 0; if (++cursor_y >= VGA_HEIGHT) vga_scroll_up(1); }
        return;
    }
    if (c == '\b') {
        if (cursor_x > 0) { cursor_x--; VGA_MEMORY[cursor_y*VGA_WIDTH+cursor_x] = (colorattr<<8)|' '; }
        update_cursor();
        return;
    }
    if (cursor_x >= VGA_WIDTH) { cursor_x = 0; if (++cursor_y >= VGA_HEIGHT) vga_scroll_up(1); }
    VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x++] = (colorattr << 8) | (uint8_t)c;
    update_cursor();
}

void vga_puts(const char *s) {
    while (*s) vga_putc(*s++);
}

void vga_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEMORY[i] = (colorattr << 8) | ' ';
    cursor_x = cursor_y = 0;
    update_cursor();
}

void vga_set_cursor(int x, int y) {
    cursor_x = x; cursor_y = y;
    update_cursor();
}

void vga_init(void) {
    serial_init();
    /* Тёмно-бирюзовый фон Deep Teal + светлый текст — «вода» Virenta */
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_banner(const char *title, const char *subtitle) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_set_cursor(0, 0);
    vga_puts(title);
    if (subtitle) {
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_puts("\n");
        vga_puts(subtitle);
    }
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("\n\n");
}

/* --- kprintf: экран + serial --- */
static void out_both(void *ctx, char c) {
    (void)ctx;
    vga_putc(c);
    serial_putc(c);
}

int kprintf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vfwrite(out_both, 0, fmt, ap);
    va_end(ap);
    return n;
}

void kputc(char c) { out_both(0, c); }

/* Чтение строки с клавиатуры через BIOS-less polling драйвера клавиатуры
   реализовано в shell.c; здесь только вывод. */
