#include <limits.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const char *numcodes16 =
		"0123456789abcdef";
static const char *numcodes32 =
		"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

int abs(int n)
{
	int const mask = n >> (sizeof(int) * CHAR_BIT - 1);
	return (n + mask) ^ mask;
}

char *unsigned_itoa(unsigned int value, char *str, int base)
{
	char* ptr = str, *ptr1 = str, tmp_char;
	unsigned int tmp_value;

	assert(base >= 2 && base <= 32);

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


char *itoa(int value, char *str, int base)
{
	char* ptr = str, *ptr1 = str, tmp_char;
	int tmp_value;

	assert(base >= 2 && base <= 32);

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

int atoi(const char *str)
{
	return (int)strtol(str, NULL, 10);
}

long int strtol(const char *str, char **endptr, int base)
{
	size_t i;
	long int value = 1;
	long int sign = 1;
	long int number = 0;

	/* Somewhat limited support atm. */
	assert(endptr == NULL);
	assert(base >= 2 && base <= 16);

	/* Remove whitespaces. */
	while (*str == ' ' || *str == '\t' || *str == '\n')
		str++;

	if (*str == '\0')
	{
		return 0;
	}

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		sign = 1;
		str++;
	}

	for (i = 0; i < strlen(str) - 1; i++)
		value *= base;

	while (*str != '\0' && strchr(numcodes16, *str) != NULL)
	{
		number += (long int)(strchr(numcodes16, *str) - numcodes16) * value;
		value /= base;
		str++;
	}

	return number * sign;
}
