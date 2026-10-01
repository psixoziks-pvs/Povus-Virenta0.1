/* ============================================================
 * Povus:Virenta — printf.c
 * Минималистичный, но полноценный форматчик для ядра.
 * ============================================================ */
#include "printf.h"
#include "string.h"

#define PF_ZEROPAD  1
#define PF_LEFT     2
#define PF_UPPER    4

static int emit_char(putchar_fn fn, void *ctx, int n, char c) {
    if (fn) fn(ctx, c);
    else if (n >= 0) { /* buf mode handled by caller wrapper */ }
    return 1;
}

static int putnum(putchar_fn fn, void *ctx, unsigned long long val, int base,
                  int upper, int flags, int width) {
    char buf[32];
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0, n = 0;
    if (!val) buf[i++] = '0';
    while (val) { buf[i++] = digits[val % base]; val /= base; }
    int pad = width - i;
    if (!(flags & PF_LEFT))
        while (pad-- > 0) { if (fn) fn(ctx, (flags & PF_ZEROPAD) ? '0' : ' '); n++; }
    while (i--) { if (fn) fn(ctx, buf[i]); n++; }
    if (flags & PF_LEFT)
        while (pad-- > 0) { if (fn) fn(ctx, ' '); n++; }
    return n;
}

static int putstr(putchar_fn fn, void *ctx, const char *s, int flags, int width) {
    int len = s ? (int)strlen(s) : 6;
    if (!s) s = "(null)";
    int pad = width - len, n = 0;
    if (!(flags & PF_LEFT)) while (pad-- > 0) { if (fn) fn(ctx, ' '); n++; }
    for (int i = 0; i < len; i++) { if (fn) fn(ctx, s[i]); n++; }
    if (flags & PF_LEFT) while (pad-- > 0) { if (fn) fn(ctx, ' '); n++; }
    return n;
}

int vfwrite(putchar_fn fn, void *ctx, const char *fmt, va_list ap) {
    int n = 0;
    while (*fmt) {
        if (*fmt != '%') { if (fn) fn(ctx, *fmt); n++; fmt++; continue; }
        fmt++;
        int flags = 0, width = 0;
        for (;; ) {
            if (*fmt == '-')      { flags |= PF_LEFT; fmt++; }
            else if (*fmt == '0') { flags |= PF_ZEROPAD; fmt++; }
            else break;
        }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == 'l' || *fmt == 'z') fmt++;
        switch (*fmt) {
        case 'c': { char c = (char)va_arg(ap, int);
                    if (fn) fn(ctx, c); n++; break; }
        case 'd': case 'i': n += putnum(fn, ctx, (unsigned long long)va_arg(ap, int), 10, 0, flags, width); break;
        case 'u':          n += putnum(fn, ctx, va_arg(ap, unsigned int), 10, 0, flags, width); break;
        case 'x':          n += putnum(fn, ctx, va_arg(ap, unsigned int), 16, 0, flags, width); break;
        case 'X':          n += putnum(fn, ctx, va_arg(ap, unsigned int), 16, PF_UPPER, flags, width); break;
        case 's':          n += putstr(fn, ctx, va_arg(ap, const char *), flags, width); break;
        case 'p':          n += putstr(fn, ctx, "0x", 0, 0);
                           n += putnum(fn, ctx, (unsigned long long)va_arg(ap, void *), 16, 0, PF_ZEROPAD, 12); break;
        case '%': if (fn) fn(ctx, '%'); n++; break;
        default:  if (fn) fn(ctx, '%'); n++; if (*fmt && fn) fn(ctx, *fmt); n++; break;
        }
        if (*fmt) fmt++;
    }
    return n;
}

/* Буферизированный вариант: fn пишет в строку */
typedef struct { char *buf; size_t size; size_t pos; } strctx;
static void str_putc(void *vctx, char c) {
    strctx *s = vctx;
    if (s->pos + 1 < s->size) s->buf[s->pos] = c;
    s->pos++;
}

int vscprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    strctx s = { buf, size, 0 };
    int n = vfwrite(str_putc, &s, fmt, ap);
    if (size) buf[ (s.pos < size ? s.pos : size - 1) ] = 0;
    return n;
}

int scprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vscprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int fwrite(putchar_fn fn, void *ctx, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vfwrite(fn, ctx, fmt, ap);
    va_end(ap);
    return n;
}
