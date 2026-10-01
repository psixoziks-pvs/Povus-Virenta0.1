; ============================================================
;  Povus:Virenta — cpu.asm (x86_64, NASM, format elf64)
;  Стабы прерываний, переключение контекста, загрузка GDT/TSS.
; ============================================================

%macro STUB_NOERR 1
isr_stub_%1:
    push qword 0              ; псевдо error_code (выравнивание кадра)
    push qword %1             ; int_no
    jmp isr_common
%endmacro

%macro STUB_ERR 1
isr_stub_%1:                  ; CPU уже положил error_code на стек
    push qword %1             ; int_no
    jmp isr_common
%endmacro

section .text
global isr_stub_table
global isr_return_from_frame
global switch_to
global load_gdt
global load_tss
global read_cr2
global wbinvd_impl

extern irq_dispatch

; --- Общий вход: полный дамп регистров -> registers_t -> C-диспетчер ---
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov  ax, ds
    push rax
    mov  ax, 0x10             ; kernel data selector
    mov  ds, ax
    mov  es, ax
    mov  rdi, rsp             ; registers_t *regs
    cld
    call irq_dispatch
    pop  rax
    mov  ds, ax
    mov  es, ax
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  r11
    pop  r10
    pop  r9
    pop  r8
    pop  rbp
    pop  rdi
    pop  rsi
    pop  rdx
    pop  rcx
    pop  rax
    add  rsp, 16              ; убрать int_no + error_code
    iretq

; --- Генерация 256 стабов ---
%assign i 0
%rep 256
%if (i==1 || i==3 || (i>=8 && i<=14) || i==17 || (i>=20 && i<=30))
STUB_ERR i
%else
STUB_NOERR i
%endif
%assign i i+1
%endrep

; Таблица адресов стабов: idt.c читает &isr_stub_table[i*16] (stride 16 байт).
section .rodata
align 16
isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
    dq 0
%assign i i+1
%endrep

; --- Выход из обработчика в процесс (ring3): восстановление кадра + iretq ---
; void isr_return_from_frame(registers_t *frame)
isr_return_from_frame:
    mov  rsp, rdi
    pop  rax
    mov  ds, ax
    mov  es, ax
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  r11
    pop  r10
    pop  r9
    pop  r8
    pop  rbp
    pop  rdi
    pop  rsi
    pop  rdx
    pop  rcx
    pop  rax
    add  rsp, 16
    iretq

; --- Переключение контекста ---
; void switch_to(uint64_t *old_rsp, uint64_t *new_rsp)
switch_to:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov  [rdi], rsp           ; old->rsp = текущий rsp
    mov  rsp, [rsi]           ; rsp = new->rsp
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbp
    pop  rbx
    ret

; --- void load_gdt(uint64_t gdtr_linear_addr) ---
; GDTR-структура {uint16 limit; uint64 base} лежит по адресу в rdi.
; Перезагрузка CS выполняется через far return (o64 retf).
load_gdt:
    lgdt [rdi]
    lea  rax, [rel .reload]
    push qword 0x08           ; kernel code selector
    push rax
    o64 retf
.reload:
    mov  ax, 0x10             ; kernel data selector
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    xor  eax, eax
    ret

; --- void load_tss(uint16_t selector) ---
load_tss:
    mov  ax, di
    ltr  ax
    ret

; --- uint64_t read_cr2(void) — адрес faults для page fault handler ---
read_cr2:
    mov  rax, cr2
    ret

wbinvd_impl:
    wbinvd
    ret
