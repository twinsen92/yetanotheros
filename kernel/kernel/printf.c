/* printf.c - generic printf implementation */
#include <kernel/addr.h>
#include <kernel/cdefs.h>
#include <kernel/utils.h>

/* Utility functions */
static const char *numcodes32 =
		"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

static char *unsigned_itoa(unsigned int value, char *str, int base)
{
	char* ptr = str, *ptr1 = str, tmp_char;
	unsigned int tmp_value;

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = numcodes32[35 + (tmp_value - value * base)];
	} while (value);

	*ptr-- = '\0';

	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return str;
}

static char *itoa(int value, char *str, int base)
{
	char* ptr = str, *ptr1 = str, tmp_char;
	int tmp_value;

	if (base < 2 && base > 32)
	{
		*str = '\0';
		return "bad base";
	}

	if (base != 10)
		return unsigned_itoa(value, str, base);

	do
	{
		tmp_value = value;
		value /= base;
		*ptr++ = numcodes32[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0)
		*ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr)
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}

	return str;
}

static bool print(void (*putchar)(char c), const char* data, size_t length)
{
	for (size_t i = 0; i < length; i++)
		putchar(data[i]);
	return true;
}

int generic_printf(void (*putchar)(char c), const char* restrict format, va_list parameters)
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
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c')
		{
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, &c, sizeof(c)))
				return -1;
			written++;
		}
		else if (*format == 's')
		{
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = kstrlen(str);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'd')
		{
			format++;
			int i = va_arg(parameters, int);
			char *str = itoa(i, buf, 10);
			size_t len = kstrlen(str);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'u')
		{
			format++;
			unsigned int i = va_arg(parameters, unsigned int);
			char *str = unsigned_itoa(i, buf, 10);
			size_t len = kstrlen(str);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'x')
		{
			format++;
			unsigned int i = va_arg(parameters, unsigned int);
			/* TODO: What about uint64_t? */
			char *str = unsigned_itoa(i, buf, 16);
			size_t len = kstrlen(str);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, str, len))
				return -1;
			written += len;
		}
		else if (*format == 'p')
		{
			format++;
			vaddr_t p = va_arg(parameters, vaddr_t);
			/* TODO: This won't work in 64-bit mode. */
			char *str = unsigned_itoa((unsigned int)p, buf, 16);
			size_t len = kstrlen(str);
			int zeroes = sizeof(vaddr_t) * 2 - len;
			if (maxrem < len + zeroes)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			for (int i = 0; i < zeroes; i++)
				if (!print(putchar, "0", 1))
					return -1;
			if (!print(putchar, str, len))
				return -1;
			written += len;
		}
		else
		{
			format = format_begun_at;
			size_t len = kstrlen(format);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(putchar, format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	return written;
}
