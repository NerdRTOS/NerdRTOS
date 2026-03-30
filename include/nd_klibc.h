#ifndef __ND_KLIBC_H__
#define __ND_KLIBC_H__

#include <stdarg.h>
#include <stddef.h>

#include "nd_def.h"

char *nd_strtok_r(char *str, const char *delim, char **saveptr);
// int nd_vsprintf(char *buf, const char *fmt, va_list args);
int nd_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
// int nd_sprintf(char *buf, const char *fmt, ...);
int nd_snprintf(char *buf, size_t size, const char *fmt, ...);

// char *nd_strcpy(char *dest, const char *src);
char *nd_strncpy(char *dest, const char *src, size_t count);
nd_int32_t nd_strcmp(const char *cs, const char *ct);
// int nd_strncmp(const char *cs, const char *ct, size_t count);
nd_size_t nd_strlen(const char *s);
// size_t nd_strnlen(const char *s, size_t count);
// char *nd_strstr(const char *s1, const char *s2);
// char *nd_strnsrt(const char *s1, const char *s2, size_t len);

// int nd_memcmp(const void *cs, const void *ct, size_t count);
// void *nd_memchr(const void *s, int c, size_t n);
void *nd_memcpy(void *dest, const void *src, size_t count);
// void *nd_memmove(void *dest, const void *src, size_t count);
void *nd_memset(void *s, int c, size_t count);
// int nd_printf(const char *fmt, ...);

#endif
