/* ============================================================
 * Povus:Virenta — VMM: виртуальная память (4-level paging)
 * Ядро использует identity-отображение первых 4 ГиБ + direct map.
 * ============================================================ */
#ifndef KERNEL_MM_VMM_H
#define KERNEL_MM_VMM_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE   4096
#define PAGE_FLAG_PRESENT  0x01
#define PAGE_FLAG_WRITE    0x02
#define PAGE_FLAG_USER     0x04
#define PAGE_FLAG_PS       0x80        /* huge page */
#define PAGE_FLAG_NX       (1ull << 63)

void     vmm_init(void);
uint64_t vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void     vmm_unmap_page(uint64_t virt);
bool     vmm_is_mapped(uint64_t virt);
uint64_t vmm_virt_to_phys(uint64_t virt);
void     vmm_switch_address_space(uint64_t pml4_phys);
uint64_t vmm_kernel_pml4(void);
uint64_t vmm_clone_address_space(void);      /* копия kernel-части для процесса */
void     vmm_map_mmio(uint64_t phys, uint64_t size);   /* отобразить MMIO в high area */
uint64_t vmm_high_virt_for(uint64_t phys);             /* direct-map адрес */

/* Обработчик page fault (регистрируется ядром) */
void page_fault_handler(void *regs_void);

#endif
