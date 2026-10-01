/* ============================================================
 * Povus:Virenta — irq.c
 * PIC 8259 + диспетчер прерываний внешнего оборудования.
 * ============================================================ */
#include "idt.h"
#include "isr.h"
#include "io.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/screen/vga.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20

static irq_handler_t handlers[48];

/* Векторы: исключения 0..31, IRQ0..15 -> 32..47 */
void pic_remap(int off1, int off2) {
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, off1); io_wait();
    outb(PIC2_DATA, off2); io_wait();
    outb(PIC1_DATA, 4);  io_wait();   /* cascade: IRQ2 на slave */
    outb(PIC2_DATA, 2);  io_wait();
    outb(PIC1_DATA, 0xFF);            /* маска: всё выкл до регистрации */
    outb(PIC2_DATA, 0xFF);
}

void pic_enable_irq(uint8_t irq) {
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) & ~(1 << (irq % 8)));
}

void pic_disable_irq(uint8_t irq) {
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) | (1 << (irq % 8)));
}

void pic_end_of_irq(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void irq_set_handler(uint8_t irq, irq_handler_t h) {
    handlers[irq] = h;
    pic_enable_irq(irq);
}

void irq_clear_handler(uint8_t irq) {
    handlers[irq] = 0;
    pic_disable_irq(irq);
}

/* Общий вход из ассемблерных стабов (см. cpu.asm) */
void irq_dispatch(registers_t *regs) {
    if (regs->int_no >= 32 && regs->int_no < 48) {
        uint8_t irq = regs->int_no - 32;
        if (handlers[irq]) handlers[irq](regs);
        pic_end_of_irq(irq);
    } else if (regs->int_no < 32) {
        static const char *names[] = {
            "Division Error","Debug","Non-Maskable Interrupt","Breakpoint",
            "Overflow","Bound Range Exceeded","Invalid Opcode","Device Not Available",
            "Double Fault","Coprocessor Segment Overrun","Invalid TSS","Segment Not Present",
            "Stack-Segment Fault","General Protection Fault","Page Fault","Reserved",
            "x87 Floating-Point Exception","Alignment Check","Machine Check",
            "SIMD Floating-Point Exception","Virtualization Exception",
            "Control Protection Exception","","","",
            "Hypervisor Injection","","VMM Communication Exception","","Security Exception"
        };
        const char *name = regs->int_no < 32 ? names[regs->int_no] : "Unknown";
        panic(name, regs);
    }
}
