/* ============================================================
 * Povus:Virenta — свой printf (без libc)
 * Поддержка: %c %d %u %x %X %s %p %% и флаги '-', ширины.
 * Вывод идёт через callback — можно писать в VGA, framebuffer
 * или serial-порт.
 * ============================================================ */
#ifndef KERNEL_LIB_PRINTF_H
#define KERNEL_LIB_PRINTF_H

#include <stdarg.h>
#include <stddef.h>

typedef void (*putchar_fn)(void *ctx, char c);

int vscprintf(char *buf, size_t size, const char *fmt, va_list ap);
int scprintf(char *buf, size_t size, const char *fmt, ...);
int vfwrite(putchar_fn fn, void *ctx, const char *fmt, va_list ap);
int fwrite(putchar_fn fn, void *ctx, const char *fmt, ...);

#endif
