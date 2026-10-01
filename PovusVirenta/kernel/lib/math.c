/* ============================================================
 * Povus:Virenta — math.c
 * ============================================================ */
#include "math.h"

int imin(int a, int b) { return a < b ? a : b; }
int imax(int a, int b) { return a > b ? a : b; }
int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

uint32_t isqrt(uint32_t x) {
    uint32_t r = 0, b = 1u << 30;
    while (b > x) b >>= 2;
    while (b) { if (x >= r + b) { x -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; }
    return r;
}

uint64_t div_round_up(uint64_t a, uint64_t b) { return (a + b - 1) / b; }

fix16_t fix16_smoothstep(fix16_t t) {
    if (t <= 0) return 0;
    if (t >= FIX16_ONE) return FIX16_ONE;
    /* s = t*t*(3 - 2t) in Q16.16 */
    fix16_t tt = fix16_mul(t, t);
    fix16_t three_minus_2t = 3 * FIX16_ONE - 2 * t;
    return fix16_mul(tt, three_minus_2t);
}

uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *state = x;
    return x;
}
