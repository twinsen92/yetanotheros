#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* abort.c */
void abort(void);

/* exit.c */
void exit(int status);
int atexit(void (*func)(void));

/* heap.c */
void *calloc(size_t num, size_t size);
void *malloc(size_t size);
void free(void *ptr);

/* convert.c */
int abs(int n);
char *itoa(int value, char *str, int base);
char *unsigned_itoa(unsigned int value, char *str, int base);
int atoi(const char *str);
long int strtol(const char *str, char **endptr, int base);

/* env.c */
char *getenv(const char *name);

#ifdef __cplusplus
}
#endif

#endif
