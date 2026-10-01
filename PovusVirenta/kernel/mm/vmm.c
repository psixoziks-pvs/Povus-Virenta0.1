/* ============================================================
 * Povus:Virenta — vmm.c
 * Страничная таблица PML4 → PDPT → PD → PT.
 * Identity map до 4 ГиБ huge-страницами (2 МиБ), плюс direct map
 * по смещению 0xFFFF800000000000 (для доступа к любой физ. памяти).
 * ============================================================ */
#include "vmm.h"
#include "mm/pmm.h"
#include "arch/x86_64/io.h"
#include "lib/string.h"
#include "drivers/screen/vga.h"

extern char __kernel_end[];

static uint64_t *pml4;            /* ядро (identity) */
static uint64_t  kernel_pml4_phys;

/* Direct map base: -512GiB (0xFFFFFF8000000000 в sign-extended)... 
   Используем классический базис: индекс PML4[511] и [510]. */
#define DM_BASE  0xFFFF888000000000ull   /* PML4[511]->PDPT[..]: упрощённый смещённый маппинг */

static uint64_t *alloc_table(void) {
    uint64_t p = pmm_alloc_page();
    if (!p) { kprintf("vmm: OOM allocating table\n"); for(;;) hlt(); }
    memset((void *)p, 0, PAGE_SIZE);
    return (uint64_t *)p;
}

static uint64_t *next_level(uint64_t *tab, int idx, bool create) {
    if (!(tab[idx] & PAGE_FLAG_PRESENT)) {
        if (!create) return 0;
        uint64_t *t = alloc_table();
        tab[idx] = (uint64_t)(uintptr_t)t | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    }
    return (uint64_t *)(uintptr_t)(tab[idx] & 0x000FFFFFFFFFF000ull);
}

void vmm_init(void) {
    /* Выделяем root таблицы из низких 4 ГиБ (identity уже накроет) */
    uint64_t pml4_phys = pmm_alloc_page();
    uint64_t pdpt_phys = pmm_alloc_page();
    pml4 = (uint64_t *)pml4_phys;
    memset(pml4, 0, PAGE_SIZE);
    memset((void *)pdpt_phys, 0, PAGE_SIZE);
    pml4[0] = pdpt_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;

    /* Identity map 0..4GiB через 2MiB huge pages: 4 PD по 512 записей */
    for (int i = 0; i < 4; i++) {
        uint64_t pd_phys = pmm_alloc_page();
        uint64_t *pd = (uint64_t *)pd_phys;
        memset(pd, 0, PAGE_SIZE);
        ((uint64_t *)pdpt_phys)[i] = pd_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
        for (int j = 0; j < 512; j++) {
            uint64_t addr = ((uint64_t)i << 30) | ((uint64_t)j << 21);
            pd[j] = addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_PS;
        }
    }

    /* Загрузить CR3 */
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
    kernel_pml4_phys = pml4_phys;

    /* NX-enable: EFER.LME уже включён загрузчиком; ставим NXE (bit11) */
    uint64_t efer;
    __asm__ volatile("rdmsr" : "=a"(efer) : "c"(0xC0000080));
    efer |= (1ull << 11);
    __asm__ volatile("wrmsr" :: "c"(0xC0000080), "a"(efer), "d"(efer >> 32));
}

uint64_t vmm_kernel_pml4(void) { return kernel_pml4_phys; }

uint64_t vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *tab = pml4;
    uint64_t va = virt;
    int i4 = (va >> 39) & 0x1FF, i3 = (va >> 30) & 0x1FF,
        i2 = (va >> 21) & 0x1FF, i1 = (va >> 12) & 0x1FF;
    uint64_t *pdpt = next_level(tab, i4, true);
    uint64_t *pd   = next_level(pdpt, i3, true);
    uint64_t *pt   = next_level(pd, i2, true);
    pt[i1] = phys | flags | PAGE_FLAG_PRESENT;
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
    return phys;
}

void vmm_unmap_page(uint64_t virt) {
    uint64_t *tab = pml4;
    uint64_t va = virt;
    int i4 = (va >> 39) & 0x1FF, i3 = (va >> 30) & 0x1FF,
        i2 = (va >> 21) & 0x1FF, i1 = (va >> 12) & 0x1FF;
    uint64_t *pdpt = next_level(tab, i4, false); if (!pdpt) return;
    uint64_t *pd   = next_level(pdpt, i3, false); if (!pd) return;
    uint64_t *pt   = next_level(pd, i2, false); if (!pt) return;
    pt[i1] = 0;
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

bool vmm_is_mapped(uint64_t virt) { return vmm_virt_to_phys(virt) != (uint64_t)-1; }

uint64_t vmm_virt_to_phys(uint64_t virt) {
    uint64_t va = virt;
    uint64_t *tab = pml4;
    uint64_t *pdpt = next_level(tab, (va>>39)&0x1FF, false); if (!pdpt) return -1;
    uint64_t *pd   = next_level(pdpt, (va>>30)&0x1FF, false); if (!pd) return -1;
    if ((pd[(va>>21)&0x1FF] & PAGE_FLAG_PS))
        return (pd[(va>>21)&0x1FF] & 0x000FFFFFFFE00000ull) | (va & 0x1FFFFF);
    uint64_t *pt = next_level(pd, (va>>21)&0x1FF, false); if (!pt) return -1;
    if (!(pt[(va>>12)&0x1FF] & PAGE_FLAG_PRESENT)) return -1;
    return (pt[(va>>12)&0x1FF] & 0x000FFFFFFFFFF000ull) | (va & 0xFFF);
}

void vmm_switch_address_space(uint64_t pml4_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}

uint64_t vmm_clone_address_space(void) {
    /* Копируем только PML4 и первый PDPT (identity 0..4GiB) — процесс
       получает своё адресное пространство в нижних 2 ГиБ. */
    uint64_t new_pml4_phys = pmm_alloc_page();
    uint64_t *np = (uint64_t *)new_pml4_phys;
    memcpy(np, pml4, PAGE_SIZE);
    /* общие таблицы уровня 2/1 переиспользуются (ядро+общая RAM);
       для полной изоляции процессов нужен COW — этап 0.9 */
    return new_pml4_phys;
}

void vmm_map_mmio(uint64_t phys, uint64_t size) {
    /* В identity-карте 0..4GiB MMIO уже доступен напрямую */
    (void)phys; (void)size;
}

uint64_t vmm_high_virt_for(uint64_t phys) {
    return phys;   /* identity: virt == phys в текущей конфигурации */
}

/* --- Page fault --- */
extern uint64_t read_cr2(void);
void page_fault_handler(void *regs_void) {
    typedef struct { uint64_t rip; uint64_t cr2_; } minimal;
    (void)regs_void;
    uint64_t addr = read_cr2();
    kprintf("\nPage fault at %x (rip nearby)\n", (uint32_t)addr);
    for (;;) hlt();
}
