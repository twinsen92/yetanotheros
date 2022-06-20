#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#include <impl/stdio.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define STDIN_N

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fflush(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
