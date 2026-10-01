/* ============================================================
 * Povus:Virenta — kheap.c
 * Куча: блоки с заголовком {magic, size, used}, first-fit,
 * слияние соседних свободных блоков (coalescing).
 * Рост кучи — страницами из PMM в области HEAP_START..HEAP_END.
 * ============================================================ */
#include "kheap.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "lib/string.h"
#include "drivers/screen/vga.h"

#define HEAP_START 0x1000000ull          /* 16 MiB */
#define HEAP_MAX   0x4000000ull          /* до 64 MiB heap */
#define MAGIC      0xDEADBEEF20260101ull

struct blk {
    uint64_t magic;
    uint32_t size;        /* полезная нагрузка */
    uint8_t  used;
    uint8_t  pad[3];
};

static uint64_t heap_cur;         /* текущая граница выделенной памяти */
static uint64_t heap_used;        /* байт занято пользователями */
static bool inited;

void kheap_init(void) {
    heap_cur = HEAP_START + sizeof(struct blk);
    struct blk *first = (struct blk *)HEAP_START;
    first->magic = MAGIC; first->size = 0; first->used = 1; /* страж */
    inited = true;
}

static void *extend(uint32_t need) {
    uint32_t sz = (need + sizeof(struct blk) + PAGE_SIZE - 1) & ~(uint32_t)(PAGE_SIZE - 1);
    if (heap_cur + sz > HEAP_MAX) return 0;
    /* Страницы уже identity-mapped и помечены занятыми в PMM при init?
       Нет: резервируем через pmm_alloc_page и отображаем на heap_cur. */
    for (uint32_t off = 0; off < sz; off += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page();
        if (!phys) return 0;
        vmm_map_page(heap_cur + off, phys, PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE);
    }
    heap_cur += sz;
    return (void *)(heap_cur - sz);
}

void *kmalloc(size_t size) {
    if (!inited || !size) return 0;
    size = (size + 15) & ~15ull;
    /* проход по блокам */
    uint64_t p = HEAP_START;
    while (p < heap_cur) {
        struct blk *b = (struct blk *)p;
        if (b->magic != MAGIC) break;
        if (!b->used && b->size >= size) {
            b->used = 1;
            heap_used += b->size;
            /* хвост остатка — новый свободный блок */
            if (b->size >= size + sizeof(struct blk) + 16) {
                struct blk *n = (struct blk *)(p + sizeof(struct blk) + size);
                n->magic = MAGIC;
                n->size = b->size - size - sizeof(struct blk);
                n->used = 0;
                b->size = size;
            }
            return (void *)(p + sizeof(struct blk));
        }
        p += sizeof(struct blk) + b->size;
    }
    /* не нашли — выделяем новый блок на границе */
    void *region = extend((uint32_t)(size + sizeof(struct blk)));
    if (!region) { kprintf("kmalloc: OOM (%u bytes)\n", (unsigned)size); return 0; }
    struct blk *b = region;
    b->magic = MAGIC; b->size = (uint32_t)size; b->used = 1;
    heap_used += size;
    return (char *)region + sizeof(struct blk);
}

void *kcalloc(size_t n, size_t sz) {
    size_t total = n * sz;
    void *p = kmalloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void kfree(void *p) {
    if (!p) return;
    struct blk *b = (struct blk *)((char *)p - sizeof(struct blk));
    if (b->magic != MAGIC || !b->used) return;   /* двойное освобождение/порча */
    b->used = 0;
    heap_used -= b->size;
    /* coalesce со следующим свободным */
    uint64_t nxt = (uint64_t)b + sizeof(struct blk) + b->size;
    if (nxt < heap_cur) {
        struct blk *n = (struct blk *)nxt;
        if (n->magic == MAGIC && !n->used) b->size += sizeof(struct blk) + n->size;
    }
}

void *krealloc(void *ptr, size_t newsz) {
    if (!ptr) return kmalloc(newsz);
    struct blk *b = (struct blk *)((char *)ptr - sizeof(struct blk));
    if (b->size >= newsz) return ptr;
    void *np = kmalloc(newsz);
    if (!np) return 0;
    memcpy(np, ptr, b->size);
    kfree(ptr);
    return np;
}

void kheap_stats(size_t *total, size_t *used, size_t *free_bytes) {
    size_t t = 0, u = 0;
    uint64_t p = HEAP_START;
    while (p < heap_cur) {
        struct blk *b = (struct blk *)p;
        if (b->magic != MAGIC) break;
        t += sizeof(struct blk) + b->size;
        if (b->used) u += b->size;
        p += sizeof(struct blk) + b->size;
    }
    if (total) *total = t;
    if (used) *used = u;
    if (free_bytes) *free_bytes = t - u;
}
