#ifndef _STDIO_NPRINTF_H
#define _STDIO_NPRINTF_H 1

#include <stddef.h>

struct generic_printer
{
	void *opaque;

	int (*put_one)(struct generic_printer *printer, char c);
	int (*put_many)(struct generic_printer *printer, const char *str, size_t len);
};

int generic_nprintf(struct generic_printer *printer, char *dest, int remaining, const char *format, va_list parameters);

#endif
