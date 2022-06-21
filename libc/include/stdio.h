#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>

#include <impl/stdio.h>

#include <yaos2/kernel/defs.h>

#define EOF (-1)

#define BUFSIZ 128

#ifdef __cplusplus
extern "C" {
#endif

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define STDIN_N

/* String printing. */

int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list arg);
int snprintf(char *str, size_t num, const char *format, ...);
int vsnprintf(char *str, size_t num, const char *format, va_list arg);

/* Stream operations. */

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
void setbuf(FILE *stream, char *buf);

int fseek(FILE *stream, long int offset, int origin);
long int ftell(FILE *stream);

int fputc(int character, FILE *stream);
int fputs(const char *str, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fflush(FILE *stream);

int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list arg);

/* Stdout convenience functions. */

int putchar(int);
int puts(const char*);
int printf(const char *format, ...);
int vprintf(const char *format, va_list arg);

#ifdef __cplusplus
}
#endif

#endif
