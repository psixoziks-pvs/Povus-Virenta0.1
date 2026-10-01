/* ============================================================
 * Povus:Virenta — IDT (Interrupt Descriptor Table)
 * ============================================================ */
#ifndef KERNEL_ARCH_IDT_H
#define KERNEL_ARCH_IDT_H

#include <stdint.h>

typedef struct registers {
    uint64_t ds;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rbx, rax, rcx, rdx, rsi, rdi;
    uint64_t int_no, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

typedef void (*irq_handler_t)(registers_t *regs);

void idt_init(void);
void irq_set_handler(uint8_t irq, irq_handler_t h);
void irq_clear_handler(uint8_t irq);
void pic_remap(int offset_icw1, int offset_icw2);
void pic_end_of_irq(uint8_t irq);
void pic_disable(void);

#endif
