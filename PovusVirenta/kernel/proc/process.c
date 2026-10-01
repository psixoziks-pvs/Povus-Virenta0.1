/* ============================================================
 * Povus:Virenta — process.c
 * Простейший round-robin планировщик ядерных потоков (kthread).
 * Каждый поток получает свой стек ядра 16 КБ из кучи.
 * ============================================================ */
#include "process.h"
#include "arch/x86_64/io.h"
#include "mm/kheap.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include "drivers/timer/pit.h"
#include "drivers/screen/vga.h"

extern void switch_to(context_t *old, context_t *new);

static process_t procs[SCHED_MAX];
static volatile int current = -1;
static bool need_resched;
static uint32_t rr_cursor;

/* Точка старта потока: fn(arg) -> при возврате помечаем DEAD и выходим */
static void kthread_trampoline(kthread_fn fn, void *arg) {
    sti();
    fn(arg);
    cli();
    procs[current].state = PROC_DEAD;
    schedule();
    for (;;) hlt();
}

/* Каркас начального контекста на стеке потока */
static void thread_bootstrap(void) { }

void scheduler_init(void) {
    memset(procs, 0, sizeof(procs));
    /* pid 0 — idle-цикл, становится текущим без переключения */
    procs[0].pid = 0;
    procs[0].state = PROC_RUNNING;
    procs[0].name = "idle";
    procs[0].is_idle = true;
    current = 0;
    kprintf("sched: round-robin ready\n");
}

int kthread_create(const char *name, kthread_fn fn, void *arg) {
    int slot = -1;
    for (int i = 1; i < SCHED_MAX; i++)
        if (procs[i].state == PROC_FREE) { slot = i; break; }
    if (slot < 0) return -1;

    process_t *p = &procs[slot];
    p->kstack = (uint64_t)kmalloc(16384);
    if (!p->kstack) return -1;
    uint64_t top = p->kstack + 16384 - 64;
    uint64_t *st = (uint64_t *)top;

    /* Подкладываем «возвращаемое» окружение так, что после switch_to
       поток окажется внутри trampoline: st[-1] = rip */
    memset(st, 0, 64);
    st[0] = (uint64_t)fn;         /* arg через rdi? кладём адрес fn как ret-адрес */
    /* Упрощённая схема: rsp указывает на слот, где лежит стартовый rip,
       а rbx/r12 несут fn/arg — trampoline заберёт их. */
    uint64_t *frame = (uint64_t *)(p->kstack + 16384 - 16*8);
    frame[15] = (uint64_t)kthread_trampoline;   /* ret адрес = start rip */
    frame[14] = (uint64_t)fn;
    frame[13] = (uint64_t)arg;
    frame[12] = 0; frame[11] = 0; frame[10] = 0; frame[9] = 0;
    frame[8]  = 0; frame[7] = 0; frame[6] = 0; frame[5] = 0;
    frame[4]  = 0; frame[3] = 0; frame[2] = 0; frame[1] = 0; frame[0] = 0;

    /* switch_to делает pop r15..rbx затем ret. Значит на стеке сверху вниз:
       [rip][r15][r14][r13][r12][rbp][rbx] => rsp указывает на rbx. */
    uint64_t *s = (uint64_t *)(p->kstack + 16384) - 7;
    s[0] = 0;                       /* rbx */
    s[1] = 0;                       /* rbp */
    s[2] = 0;                       /* r12 */
    s[3] = (uint64_t)arg;           /* r13 */
    s[4] = (uint64_t)fn;            /* r14 */
    s[5] = 0;                       /* r15 */
    s[6] = (uint64_t)kthread_trampoline; /* ret address */
    p->ctx.rsp = (uint64_t)s;
    p->ctx.rbz_unused = 0;

    p->state = PROC_READY;
    p->name = name;
    p->pid = slot;
    return slot;
}

/* Реальный trampoline использует r14=fn, r13=arg — перепишем простой вариант:
   kthread_trampoline(fn,arg) выше вызывался бы через ret, но регистры не те.
   Поэтому делаем трюк: rip = tramp_regs, которая забирает fn/arg из r14/r13. */
static void tramp_regs_impl(void);
__asm__(
".text\n"
"tramp_regs:\n"
"  mov %r14, %rdi\n"
"  mov %r13, %rsi\n"
"  jmp kthread_trampoline_setregs\n"
);
extern void kthread_trampoline_setregs(kthread_fn fn, void *arg);
void kthread_trampoline_setregs(kthread_fn fn, void *arg) {
    sti();
    fn(arg);
    cli();
    procs[current].state = PROC_DEAD;
    schedule();
    for (;;) hlt();
}

int proc_count(void) {
    int n = 0;
    for (int i = 0; i < SCHED_MAX; i++)
        if (procs[i].state != PROC_FREE && procs[i].state != PROC_DEAD) n++;
    return n;
}

process_t *current_process(void) { return &procs[current >= 0 ? current : 0]; }

void sleep_ms_sched(uint64_t ms) {
    if (ms == 0) { yield(); return; }
    procs[current].state = PROC_SLEEPING;
    procs[current].sleep_until_ms = timer_ms() + ms;
    schedule();
}

void yield(void) { need_resched = true; schedule(); }

void timer_tick_schedule(void) {
    /* разбудить спящих */
    uint64_t now = timer_ms();
    for (int i = 0; i < SCHED_MAX; i++)
        if (procs[i].state == PROC_SLEEPING && now >= procs[i].sleep_until_ms)
            procs[i].state = PROC_READY;
    need_resched = true;
}

void schedule(void) {
    if (!need_resched && procs[current].state == PROC_RUNNING) return;
    need_resched = false;
    int prev = current;
    int next = -1;
    for (int tries = 0; tries < SCHED_MAX; tries++) {
        int i = rr_cursor % SCHED_MAX;
        rr_cursor++;
        if (procs[i].state == PROC_READY) { next = i; break; }
    }
    if (next < 0) {
        /* только idle или текущий всё ещё runnable */
        if (procs[prev].state == PROC_RUNNING || procs[prev].state == PROC_READY) {
            procs[prev].state = PROC_READY;
            /* вернёмся к тому же — idle всегда готов */
            next = 0;
            if (prev == 0) { procs[0].state = PROC_RUNNING; return; }
        } else {
            next = 0;
        }
    }
    if (next == prev) { procs[next].state = PROC_RUNNING; return; }
    if (procs[prev].state == PROC_RUNNING) procs[prev].state = PROC_READY;
    current = next;
    procs[next].state = PROC_RUNNING;
    switch_to(&procs[prev].ctx, &procs[next].ctx);
}

void proc_dump(void) {
    kprintf("PID  STATE     NAME\n");
    for (int i = 0; i < SCHED_MAX; i++) {
        if (procs[i].state == PROC_FREE) continue;
        const char *st = "?";
        switch (procs[i].state) {
            case PROC_READY: st = "ready"; break;
            case PROC_RUNNING: st = "run"; break;
            case PROC_SLEEPING: st = "sleep"; break;
            case PROC_DEAD: st = "dead"; break;
            default: break;
        }
        kprintf("%3d  %-8s %s%s\n", procs[i].pid, st,
                procs[i].name ? procs[i].name : "-",
                i == current ? "  <- current" : "");
    }
}
