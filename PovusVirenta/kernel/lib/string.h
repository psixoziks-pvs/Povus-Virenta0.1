/* ============================================================
 * Povus:Virenta — string.h (своя библиотека, без libc)
 * ============================================================ */
#ifndef KERNEL_LIB_STRING_H
#define KERNEL_LIB_STRING_H

#include <stdint.h>
#include <stddef.h>

void   *memset(void *dst, int c, size_t n);
void   *memcpy(void *dst, const void *src, size_t n);
void   *memmove(void *dst, const void *src, size_t n);
int     memcmp(const void *a, const void *b, size_t n);
size_t  strlen(const char *s);
size_t  strnlen(const char *s, size_t max);
char   *strcpy(char *dst, const char *src);
char   *strncpy(char *dst, const char *src, size_t n);
int     strcmp(const char *a, const char *b);
int     strncmp(const char *a, const char *b, size_t n);
int     strcasecmp(const char *a, const char *b);
char   *strchr(const char *s, int c);
char   *strrchr(const char *s, int c);
char   *strstr(const char *hay, const char *needle);
char   *strcat(char *dst, const char *src);
/* Разбор строки по разделителям (не разрушающий оригинал; копия в tok) */
int     token_split(const char *s, const char *delim, char out[][64], int max_tokens);

#endif
