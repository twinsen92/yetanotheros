
#include <yaos2/kernel/defs.h>
#include <yaos2/kernel/syscalls.h>

#include <yaos2/arch/syscall.h>

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "nprintf.h"

static int stdout_put_one(struct generic_printer *printer, char c);
static int stdout_put_many(struct generic_printer *printer, const char *str, size_t len);

static struct generic_printer stdout_printer = {
		.put_one = stdout_put_one,
		.put_many = stdout_put_many,
};

int putchar(int ic)
{
	return stdout_put_one(&stdout_printer, (char)ic);
}

int puts(const char* string)
{
	return printf("%s\n", string);
}

static int stdout_put_one(struct generic_printer *printer, char c)
{
	return stdout_put_many(printer, &c, 1);
}

static int stdout_put_many(struct generic_printer *printer, const char *str, size_t len)
{
	int ret = fwrite(str, 1, len, stdout);

	if (ret < 0)
	{
		errno = -ret;
		ret = -1;
	}

	return ret;
}

int printf(const char *format, ...)
{
	int ret;
	va_list parameters;

	va_start(parameters, format);
	ret = generic_nprintf(&stdout_printer, NULL, -1, format, parameters);
	va_end(parameters);

	return ret;
}

int vprintf(const char *format, va_list arg)
{
	return generic_nprintf(&stdout_printer, NULL, -1, format, arg);
}
