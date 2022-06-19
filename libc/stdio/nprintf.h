#ifndef _STDIO_NPRINTF_H
#define _STDIO_NPRINTF_H 1

#include <stddef.h>

struct generic_printer
{
	int (*put_one)(char c);
	int (*put_many)(const char *str, size_t len);
};

int generic_nprintf(struct generic_printer *printer, char *dest, int remaining, const char* __restrict format, va_list parameters);

#endif
