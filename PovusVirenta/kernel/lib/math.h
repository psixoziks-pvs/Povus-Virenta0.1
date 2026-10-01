/* ============================================================
 * Povus:Virenta — math.h (целочисленная математика без libm)
 * ============================================================ */
#ifndef KERNEL_LIB_MATH_H
#define KERNEL_LIB_MATH_H

#include <stdint.h>
#include <stddef.h>

int      imin(int a, int b);
int      imax(int a, int b);
int      iclamp(int v, int lo, int hi);
uint32_t isqrt(uint32_t x);
uint64_t div_round_up(uint64_t a, uint64_t b);
/* Фиксированная точка Q16.16 для плавных анимаций GUI */
typedef int32_t fix16_t;
#define FIX16_ONE   65536
static inline fix16_t fix16_from_int(int x)  { return (fix16_t)(x << 16); }
static inline int     fix16_to_int(fix16_t x){ return x >> 16; }
static inline fix16_t fix16_mul(fix16_t a, fix16_t b) { return (fix16_t)(((int64_t)a * b) >> 16); }
static inline fix16_t fix16_div(fix16_t a, fix16_t b) { return (fix16_t)(((int64_t)a << 16) / b); }
/* Плавная кривая ease-in-out (0..1 → 0..1 в Q16.16): t*t*(3-2t) */
fix16_t fix16_smoothstep(fix16_t t);
/* Простой PRNG (xorshift32) — для «пузырьков» и частиц */
uint32_t xorshift32(uint32_t *state);

#endif
