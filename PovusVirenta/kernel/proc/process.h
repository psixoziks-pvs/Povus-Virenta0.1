/* ============================================================
 * Povus:Virenta — процессы и вытесняющая многозадачность
 * Планировщик round-robin по тикам PIT.
 * ============================================================ */
#ifndef KERNEL_PROC_PROCESS_H
#define KERNEL_PROC_PROCESS_H

#include <stdint.h>
#include <stdbool.h>

#define SCHED_MAX 16

typedef enum { PROC_FREE, PROC_READY, PROC_RUNNING, PROC_SLEEPING, PROC_DEAD } proc_state_t;

/* Контекст переключения: rsp + callee-saved (согласовано с cpu.asm) */
typedef struct context {
    uint64_t rsp;
    uint64_t rbx, rbp, r12, r13, r14, r15;
} context_t;

typedef void (*kthread_fn)(void *arg);

typedef struct process {
    int      pid;
    proc_state_t state;
    context_t ctx;
    uint64_t kstack;          /* физический/виртуальный низ стека ядра */
    uint64_t sleep_until_ms;
    const char *name;
    bool is_idle;
    /* ring3 (этап 0.9): пользовательский стек и точка входа */
    uint64_t user_entry, user_rsp;
    uint64_t pml4;            /* адресное пространство (0 = ядро) */
} process_t;

void scheduler_init(void);
int  kthread_create(const char *name, kthread_fn fn, void *arg);
void yield(void);
void schedule(void);                 /* выбрать следующий процесс и switch_to */
void sleep_ms_sched(uint64_t ms);    /* сон текущего процесса */
int  proc_count(void);
process_t *current_process(void);
void proc_dump(void);                /* для команды `ps` */
void timer_tick_schedule(void);      /* вызывается из IRQ0 раз в тик */

#endif
