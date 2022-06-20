#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* memutils.c */
int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
void *memchr(const void *ptr, int value, size_t num);

/* strutils.c */
size_t strlen(const char*);
const char *strchr(const char*, int);
char *strcat(char *to, const char *from);
char *strcpy(char *to, const char *from);
char *strncpy(char *to, const char *from, size_t num);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t num);
char *strstr(const char *s1, const char *s2);

#ifdef __cplusplus
}
#endif

#endif
