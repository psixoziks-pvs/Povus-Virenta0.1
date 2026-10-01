/* ============================================================
 * Povus:Virenta — framebuffer.c
 * LFB через Bochs VBE dispi (0x1CE/0x1CF), адрес 0xE0000000
 * (identity-mapped в низких 4 ГиБ). Встроенный шрифт 8x16 ASCII.
 * ============================================================ */
#include "framebuffer.h"
#include "arch/x86_64/io.h"
#include "lib/string.h"
#include "lib/math.h"
#include "drivers/screen/vga.h"

#define VBE_DISPI_IOWR   0x1CE
#define VBE_DISPI_IORD   0x1CF
#define VBE_DISPI_INDEX_ID       0
#define VBE_DISPI_INDEX_XRES     1
#define VBE_DISPI_INDEX_YRES     2
#define VBE_DISPI_INDEX_BPP      3
#define VBE_DISPI_INDEX_ENABLE   4
#define VBE_DISPI_INDEX_BANK     5
#define VBE_DISPI_INDEX_VIRT_WIDTH  6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 7
#define VBE_DISPI_INDEX_X_OFFSET    8
#define VBE_DISPI_INDEX_Y_OFFSET    9
#define VBE_DISPI_DISABLED     0x00
#define VBE_DISPI_ENABLED      0x01
#define VBE_DISPI_LFB_ENABLED  0x40

#define FB_PHYS 0xE0000000ull

static uint32_t *fb;
static uint32_t W, H;
static bool active;

static void dispi_write(uint16_t reg, uint16_t val) {
    outw(VBE_DISPI_IOWR, reg);
    outw(VBE_DISPI_IOWR + 2, val);
}
static uint16_t dispi_read(uint16_t reg) {
    outw(VBE_DISPI_IOWR, reg);
    return inw(VBE_DISPI_IORD + 2);
}

bool fb_init(void) {
    uint16_t id = dispi_read(VBE_DISPI_INDEX_ID);
    if ((id & 0xFFF0) != 0xB0C0 && id != 0xB0C0 && id != 0xB0C1 && id != 0xB0C2 && id != 0xB0C3 && id != 0xB0C4 && id != 0xB0C5) {
        kprintf("fb: no Bochs VBE (id=%x)\n", id);
        return false;
    }
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    dispi_write(VBE_DISPI_INDEX_XRES, 1024);
    dispi_write(VBE_DISPI_INDEX_YRES, 768);
    dispi_write(VBE_DISPI_INDEX_BPP, 32);
    dispi_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    W = dispi_read(VBE_DISPI_INDEX_XRES);
    H = dispi_read(VBE_DISPI_INDEX_YRES);
    if (W == 0 || H == 0) return false;
    fb = (uint32_t *)(uintptr_t)FB_PHYS;   /* identity map 0..4GiB уже есть */
    active = true;
    kprintf("fb: %ux%u LFB @%x\n", W, H, (uint32_t)FB_PHYS);
    return true;
}

bool fb_active(void) { return active; }
uint32_t fb_width(void)  { return W; }
uint32_t fb_height(void) { return H; }

static inline uint32_t pack(rgb_t c) { return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b; }

void fb_putpixel(uint32_t x, uint32_t y, rgb_t c) {
    if (!active || x >= W || y >= H) return;
    fb[y * W + x] = pack(c);
}
rgb_t fb_getpixel(uint32_t x, uint32_t y) {
    rgb_t c = {0,0,0};
    if (!active || x >= W || y >= H) return c;
    uint32_t v = fb[y * W + x];
    c.r = v >> 16; c.g = v >> 8; c.b = v;
    return c;
}
void fb_clear(rgb_t c) {
    if (!active) return;
    uint32_t v = pack(c);
    for (uint32_t i = 0; i < W * H; i++) fb[i] = v;
}
void fb_rect(int x, int y, int w, int h, rgb_t c) {
    if (!active) return;
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= (int)H) continue;
        for (int i = 0; i < w; i++) {
            int xx = x + i;
            if (xx < 0 || xx >= (int)W) continue;
            fb[yy * W + xx] = pack(c);
        }
    }
}
void fb_blend_pixel(uint32_t x, uint32_t y, rgb_t c, uint8_t a) {
    if (!active || x >= W || y >= H) return;
    if (a >= 255) { fb[y*W+x] = pack(c); return; }
    uint32_t d = fb[y * W + x];
    uint32_t dr = d >> 16, dg = (d >> 8) & 0xFF, db = d & 0xFF;
    uint32_t na = 255 - a;
    fb[y*W+x] = ((c.r * a + dr * na) / 255) << 16 |
                ((c.g * a + dg * na) / 255) << 8 |
                ((c.b * a + db * na) / 255);
}
void fb_fill_rounded(int x, int y, int w, int h, int r, rgb_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int dx = 0, dy = 0;
            if (i < r) dx = r - i; else if (i >= w - r) dx = i - (w - r - 1);
            if (j < r) dy = r - j; else if (j >= h - r) dy = j - (h - r - 1);
            if (dx > 0 && dy > 0 && dx*dx + dy*dy > r*r) continue;
            fb_blend_pixel(x + i, y + j, c, 255);
        }
}
void fb_hgradient(int x, int y, int w, int h, rgb_t l, rgb_t rgt) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            uint8_t t = w > 1 ? (uint8_t)((uint32_t)i * 255 / (w - 1)) : 0;
            rgb_t c = { (uint8_t)(l.r + ((rgt.r - l.r) * t) / 255),
                        (uint8_t)(l.g + ((rgt.g - l.g) * t) / 255),
                        (uint8_t)(l.b + ((rgt.b - l.b) * t) / 255) };
            fb_blend_pixel(x + i, y + j, c, 255);
        }
}
void fb_vgradient_alpha(int x, int y, int w, int h, rgb_t top, rgb_t bot,
                        uint8_t at, uint8_t ab) {
    for (int j = 0; j < h; j++) {
        uint8_t t = h > 1 ? (uint8_t)((uint32_t)j * 255 / (h - 1)) : 0;
        rgb_t c = { (uint8_t)(top.r + ((bot.r - top.r) * t) / 255),
                    (uint8_t)(top.g + ((bot.g - top.g) * t) / 255),
                    (uint8_t)(top.b + ((bot.b - top.b) * t) / 255) };
        uint8_t a = (uint8_t)(at + ((ab - at) * t) / 255);
        for (int i = 0; i < w; i++) fb_blend_pixel(x + i, y + j, c, a);
    }
}
void fb_circle(int cx, int cy, int r, rgb_t c) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        fb_blend_pixel(cx+x, cy+y, c, 255); fb_blend_pixel(cx-x, cy+y, c, 255);
        fb_blend_pixel(cx+x, cy-y, c, 255); fb_blend_pixel(cx-x, cy-y, c, 255);
        fb_blend_pixel(cx+y, cy+x, c, 255); fb_blend_pixel(cx-y, cy+x, c, 255);
        fb_blend_pixel(cx+y, cy-x, c, 255); fb_blend_pixel(cx-y, cy-x, c, 255);
        y++;
        if (err < 0) err += 2*y + 1;
        else { x--; err += 2*(y - x) + 1; }
    }
}
void fb_disk(int cx, int cy, int r, rgb_t c) {
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)isqrt((uint32_t)(r*r - dy*dy));
        for (int i = -dx; i <= dx; i++) fb_blend_pixel(cx+i, cy+dy, c, 255);
    }
}
void fb_line(int x0, int y0, int x1, int y1, rgb_t c) {
    int dx = imax(x1-x0, -(x1-x0)), dy = imax(y1-y0, -(y1-y0));
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        fb_blend_pixel(x0, y0, c, 255);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}
void fb_blit_rgb(int x, int y, int w, int h, const uint8_t *src, int pitch) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            const uint8_t *p = src + j * pitch + i * 3;
            rgb_t c = { p[0], p[1], p[2] };
            fb_blend_pixel(x + i, y + j, c, 255);
        }
}
void fb_blit_rgba(int x, int y, int w, int h, const uint8_t *src, int pitch) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            const uint8_t *p = src + j * pitch + i * 4;
            rgb_t c = { p[0], p[1], p[2] };
            fb_blend_pixel(x + i, y + j, c, p[3]);
        }
}

/* --- Шрифт 8x16 (ASCII 32..126), встроенный bitmap ---
 * Сгенерирован компактно: каждый глиф — 16 байт строк.
 * Используется классический IBM-подобный узор для читаемости.
 */
static const uint8_t font_data[] = {
#include "font8x16.inc"
};

static void draw_glyph(int x, int y, char ch, rgb_t fg, rgb_t bg, bool opaque) {
    if (ch < 32 || ch > 126) ch = '?';
    const uint8_t *g = font_data + (ch - 32) * 16;
    for (int row = 0; row < 16; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) fb_blend_pixel(x+col, y+row, fg, 255);
            else if (opaque) fb_blend_pixel(x+col, y+row, bg, 255);
        }
    }
}

void fb_draw_text(int x, int y, const char *s, rgb_t fg, rgb_t bg, bool opaque) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { cx = x; y += 16; s++; continue; }
        draw_glyph(cx, y, *s++, fg, bg, opaque);
        cx += 8;
    }
}
int fb_text_width(const char *s) { return (int)strlen(s) * 8; }
