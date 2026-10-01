/* ============================================================
 * Povus:Virenta — string.c
 * Собственные строковые/мемные функции (freestanding C11).
 * ============================================================ */
#include "string.h"

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (d < s) return memcpy(d, s, n);
    d += n; s += n;
    while (n--) *--d = *--s;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *x = a, *y = b;
    for (; n--; x++, y++)
        if (*x != *y) return (int)*x - (int)*y;
    return 0;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

size_t strnlen(const char *s, size_t max) {
    size_t i = 0;
    while (i < max && s[i]) i++;
    return i;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    for (; n; a++, b++, n--)
        if (!*a || *a != *b) return (int)(uint8_t)*a - (int)(uint8_t)*b;
    return 0;
}

static int tolower_c(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

int strcasecmp(const char *a, const char *b) {
    while (*a && tolower_c((uint8_t)*a) == tolower_c((uint8_t)*b)) { a++; b++; }
    return tolower_c((uint8_t)*a) - tolower_c((uint8_t)*b);
}

char *strchr(const char *s, int c) {
    for (; *s; s++) if (*s == (char)c) return (char *)s;
    return c == 0 ? (char *)s : 0;
}

char *strrchr(const char *s, int c) {
    char *last = 0;
    for (; *s; s++) if (*s == (char)c) last = (char *)s;
    return last;
}

char *strstr(const char *hay, const char *needle) {
    size_t nl = strlen(needle);
    if (!nl) return (char *)hay;
    for (; *hay; hay++)
        if (!strncmp(hay, needle, nl)) return (char *)hay;
    return 0;
}

char *strcat(char *dst, const char *src) {
    char *d = dst + strlen(dst);
    while ((*d++ = *src++));
    return dst;
}

int token_split(const char *s, const char *delim, char out[][64], int max_tokens) {
    int count = 0;
    while (*s && count < max_tokens) {
        while (*s && strchr(delim, *s)) s++;
        if (!*s) break;
        int j = 0;
        while (*s && !strchr(delim, *s) && j < 63) out[count][j++] = *s++;
        out[count][j] = 0;
        count++;
    }
    return count;
}
