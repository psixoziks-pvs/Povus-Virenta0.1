/* ============================================================
 * Povus:Virenta — pmm.c
 * Физический аллокатор на битовой карте.
 * Карта лежит в фиксированной области HIGH_MAP (см. linker.ld):
 *   0x200000 .. 0x300000 — bitmap (1 МБ = до 32 МиБ RAM по 4КБ)
 * Расширено: поддержка до 4 ГиБ (bitmap 128 КБ при 4МиБ карт).
 * ============================================================ */
#include "pmm.h"
#include "lib/string.h"
#include "lib/math.h"

extern char __high_map_start[];      /* 0x200000 — сюда кладём карту */
extern char __kernel_end[];          /* конец образа ядра */

#define PAGE_SIZE 4096
#define BITMAP_BYTES ((uint64_t)128 * 1024)   /* 128 КБ -> 32 МиБ * 8... см. ниже */
/* 128 KiB * 8 = 1 048 576 бит * 4 KiB = 4 GiB адресуемой памяти */

static volatile uint8_t *bitmap;
static uint64_t total_pages;
static uint64_t free_count;
static uint64_t next_hint;

static inline bool bm_get(uint64_t i) { return (bitmap[i/8] >> (i%8)) & 1; }
static inline void bm_set(uint64_t i) { bitmap[i/8] |=  (uint8_t)(1u << (i%8)); }
static inline void bm_clr(uint64_t i) { bitmap[i/8] &= ~(uint8_t)(1u << (i%8)); }

void pmm_init(uint64_t max_phys_bytes) {
    bitmap = (volatile uint8_t *)__high_map_start;
    uint64_t map_bytes = BITMAP_BYTES;
    memset((void *)bitmap, 0xFF, map_bytes);       /* всё занято */

    uint64_t usable = max_phys_bytes & ~(PAGE_SIZE - 1);
    if (usable > 4ull * 1024 * 1024 * 1024) usable = 4ull * 1024 * 1024 * 1024;
    total_pages = usable / PAGE_SIZE;

    /* Освободить страницы после карты и после ядра */
    uint64_t kstart = (uint64_t)(uintptr_t)&__kernel_end;
    kstart = (kstart + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
    uint64_t low_free = (uint64_t)__high_map_start + map_bytes;
    if (low_free < kstart) low_free = kstart;
    low_free = (low_free + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

    for (uint64_t p = low_free / PAGE_SIZE; p < total_pages; p++) {
        bm_clr(p);
        free_count++;
    }
    /* Занять область карты */
    for (uint64_t p = 0x200000 / PAGE_SIZE; p < (0x200000 + map_bytes + PAGE_SIZE-1) / PAGE_SIZE && p < total_pages; p++) {
        if (!bm_get(p)) { bm_set(p); free_count--; }
    }
    /* Первые 1 МиБ всегда заняты (BIOS/low memory) */
    for (uint64_t p = 0; p < 0x100000 / PAGE_SIZE && p < total_pages; p++) bm_set(p);

    next_hint = low_free / PAGE_SIZE;
}

uint64_t pmm_alloc_page(void) {
    for (uint64_t tries = 0; tries < 2; tries++) {
        for (uint64_t p = next_hint; p < total_pages; p++) {
            if (!bm_get(p)) {
                bm_set(p);
                if (free_count) free_count--;
                next_hint = p + 1;
                return p * PAGE_SIZE;
            }
        }
        next_hint = 0;   /* второй проход с начала */
    }
    return 0;
}

void pmm_free_page(uint64_t addr) {
    uint64_t p = addr / PAGE_SIZE;
    if (p >= total_pages) return;
    if (bm_get(p)) { bm_clr(p); free_count++; if (p < next_hint) next_hint = p; }
}

uint64_t pmm_alloc_contig(uint32_t pages) {
    if (!pages) return 0;
    uint64_t run = 0;
    for (uint64_t p = 0; p < total_pages; p++) {
        if (bm_get(p)) { run = 0; continue; }
        if (++run == pages) {
            uint64_t start = p - pages + 1;
            for (uint64_t q = start; q <= p; q++) { bm_set(q); free_count--; }
            return start * PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free_contig(uint64_t addr, uint32_t pages) {
    for (uint32_t i = 0; i < pages; i++) pmm_free_page(addr + (uint64_t)i * PAGE_SIZE);
}

uint64_t pmm_total_pages(void) { return total_pages; }
uint64_t pmm_free_pages(void)  { return free_count; }

void pmm_mark_region(uint64_t start, uint64_t end) {
    uint64_t p0 = start / PAGE_SIZE, p1 = (end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = p0; p < p1 && p < total_pages; p++)
        if (!bm_get(p)) { bm_set(p); if (free_count) free_count--; }
}
