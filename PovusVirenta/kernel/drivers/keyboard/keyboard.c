/* ============================================================
 * Povus:Virenta — keyboard.c
 * Опрос IRQ1, перевод scancode -> ASCII, ring-буфер.
 * ============================================================ */
#include "keyboard.h"
#include "arch/x86_64/io.h"
#include "arch/x86_64/irq.h"
#include "lib/string.h"

static const char keymap_normal[128] = {
    0,   27,  '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.','/',
    0,   '*', 0, ' ', 0, 0,0,0,0,0,0,0,0,0,0,
    0,   0,0,0, 0,0,0, 0,0,0,0,0,0,0,0,
    '-', 0, '+', 0, 0, 0,0,0,0,0, 0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char keymap_shift[128] = {
    0,   27,  '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,   '|','Z','X','C','V','B','N','M','<','>','?',
    0,   '*', 0, ' ', 0, 0,0,0,0,0,0,0,0,0,0,
    0,   0,0,0, 0,0,0, 0,0,0,0,0,0,0,0,
    '-', 0, '+', 0, 0, 0,0,0,0,0, 0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define KBUF_SIZE 256
static volatile char kbuf[KBUF_SIZE];
static volatile int khead, ktail;
static volatile bool lshift, rshift, ctrl;
static volatile uint8_t dbg_sc[16];
static volatile int dbg_n;

static void kb_handler(registers_t *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);
    dbg_sc[dbg_n & 15] = sc; dbg_n++;

    /* release: scancode | 0x80 */
    bool up = sc & 0x80;
    sc &= 0x7F;

    if (sc == 0x2A || sc == 0x36) { lshift = rshift = !up; return; }
    if (sc == 0x1D) { ctrl = !up; return; }
    if (up) return;

    char c = (lshift || rshift) ? keymap_shift[sc] : keymap_normal[sc];
    if (ctrl && c >= 'a' && c <= 'z') c = c - 'a' + 1;  /* ^A..^Z */
    if (c) {
        int next = (khead + 1) % KBUF_SIZE;
        if (next != ktail) { kbuf[khead] = c; khead = next; }
    }
}

void keyboard_init(void) {
    khead = ktail = 0;
    irq_set_handler(1, kb_handler);
}

bool keyboard_key_available(void) { return khead != ktail; }

char keyboard_getc(void) {
    while (khead == ktail) hlt();
    char c = kbuf[ktail];
    ktail = (ktail + 1) % KBUF_SIZE;
    return c;
}

void keyboard_debug(uint8_t out[16], int *n) {
    memcpy((void *)out, (const void *)dbg_sc, 16);
    *n = dbg_n;
}
