/* ============================================================
 * Povus:Virenta — PMM: Physical Memory Manager
 * Битовая карта страниц 4 КБ.
 * ============================================================ */
#ifndef KERNEL_MM_PMM_H
#define KERNEL_MM_PMM_H

#include <stdint.h>
#include <stdbool.h>

void     pmm_init(uint64_t max_phys_bytes);
uint64_t pmm_alloc_page(void);          /* 0 = нет памяти */
void     pmm_free_page(uint64_t addr);
uint64_t pmm_alloc_contig(uint32_t pages);
void     pmm_free_contig(uint64_t addr, uint32_t pages);
uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);
/* Пометить диапазон как занятый (bootstrap-области ядра и т.п.) */
void     pmm_mark_region(uint64_t start, uint64_t end);

#endif
