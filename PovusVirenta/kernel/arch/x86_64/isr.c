/* ============================================================
 * Povus:Virenta — isr.c
 * Обработчики критических исключений и «blue screen»-панель
 * (в фирменной палитре Virenta, но честно: Kernel Bloom).
 * ============================================================ */
#include "isr.h"
#include "idt.h"
#include "io.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/screen/vga.h"

extern uint64_t read_cr2(void);

void panic(const char *msg, registers_t *regs) {
    cli();
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    kprintf("\n!!! KERNEL BLOOM (panic): %s !!!\n", msg ? msg : "?");
    if (regs) {
        kprintf("  int_no=%d err=%x rip=%x cs=%x rflags=%x\n",
                (int)regs->int_no, (uint32_t)regs->error_code,
                (uint32_t)regs->rip, (uint32_t)regs->cs, (uint32_t)regs->rflags);
        kprintf("  rsp=%x rax=%x rbx=%x rcx=%x rdx=%x\n",
                (uint32_t)regs->rsp, (uint32_t)regs->rax, (uint32_t)regs->rbx,
                (uint32_t)regs->rcx, (uint32_t)regs->rdx);
        if (regs->int_no == 14)
            kprintf("  CR2 (fault addr) = %x\n", (uint32_t)read_cr2());
    }
    kprintf("System halted. Press Ctrl+Alt+Del or reset.\n");
    for (;;) hlt();
}

/* Регистрация не нужна отдельно: все 256 векторов уже ведут в стабы,
   а диспетчер исключений живёт в irq_dispatch(). Функция оставлена
   для симметрии API и будущих кастомных обработчиков. */
void isr_register_exception_handlers(void) { }
