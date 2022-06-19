#include <limits.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaos2/kernel/errno.h>

#include "nprintf.h"

static bool print(struct generic_printer *printer,
		char *dest,
		int off,
		int *remaining,
		const char *data,
		size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		if (*remaining == 0)
			return false;

		if (dest)
			dest[off + i] = data[i];
		else
			printer->put_one(data[i]);

		(*remaining)--;
	}

	return true;
}

int generic_nprintf(struct generic_printer *printer,
		char *dest,
		int remaining,
		const char *__restrict format,
		va_list parameters)
{
	char buf[32];
	int written = 0;

	while (*format != '\0')
	{
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%')
		{
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char *format_begun_at = format++;

		if (*format == 'c')
		{
			format++;
			char c = (char)va_arg(parameters, int /* char promotes to int */);
			if (!maxrem)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, &c, sizeof(c)))
				return -1;
			written++;
		}
		else if (*format == 's')
		{
			format++;
			const char *str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'd')
		{
			format++;
			int i = va_arg(parameters, int);
			char *str = itoa(i, buf, 10);
			size_t len = strlen(str);
			if (maxrem < len)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'u')
		{
			format++;
			unsigned int i = va_arg(parameters, unsigned int);
			char *str = unsigned_itoa(i, buf, 10);
			size_t len = strlen(str);
			if (maxrem < len)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'x')
		{
			format++;
			unsigned int i = va_arg(parameters, unsigned int);
			/* TODO: What about uint64_t? */
			char *str = unsigned_itoa(i, buf, 16);
			size_t len = strlen(str);
			if (maxrem < len)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'p')
		{
			format++;
			void *p = va_arg(parameters, void*);
			/* TODO: This won't work in 64-bit mode. */
			char *str = unsigned_itoa((unsigned int)p, buf, 16);
			size_t len = strlen(str);
			int zeroes = sizeof(void*) * 2 - len;
			if (maxrem < len + zeroes)
			{
				errno = EOVERFLOW;
				return -1;
			}
			for (int i = 0; i < zeroes; i++)
				if (!print(printer, dest, written, &remaining, "0", 1))
					return -1;
			if (!print(printer, dest, written, &remaining, str, len))
				return -1;
			written += len;
		}
		else
		{
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len)
			{
				errno = EOVERFLOW;
				return -1;
			}
			if (!print(printer, dest, written, &remaining, format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	return written;
}
