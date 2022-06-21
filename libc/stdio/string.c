
#include <stdarg.h>
#include <stddef.h>

#include "nprintf.h"

int sprintf(char *str, const char *format, ...)
{
	int ret;
	va_list parameters;

	va_start(parameters, format);
	ret = generic_nprintf(NULL, str, -1, format, parameters);
	va_end(parameters);

	return ret;
}

int vsprintf(char *str, const char *format, va_list arg)
{
	return generic_nprintf(NULL, str, -1, format, arg);
}

int snprintf(char *str, size_t num, const char *format, ...)
{
	int ret;
	va_list parameters;

	va_start(parameters, format);
	ret = generic_nprintf(NULL, str, num, format, parameters);
	va_end(parameters);

	return ret;
}

int vsnprintf(char *str, size_t num, const char *format, va_list arg)
{
	return generic_nprintf(NULL, str, num, format, arg);
}
