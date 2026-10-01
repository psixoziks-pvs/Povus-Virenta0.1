/* ============================================================
 * Povus:Virenta — kheap: куча ядра (kmalloc/kfree)
 * First-fit списочный аллокатор поверх страничного PMM.
 * ============================================================ */
#ifndef KERNEL_MM_KHEAP_H
#define KERNEL_MM_KHEAP_H

#include <stddef.h>

void  *kmalloc(size_t size);
void  *kcalloc(size_t n, size_t size);
void  *krealloc(void *p, size_t newsize);
void   kfree(void *p);
void   kheap_init(void);
/* Статистика для команды `mem` */
void   kheap_stats(size_t *total, size_t *used, size_t *free_bytes);

#endif
