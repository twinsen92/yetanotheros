/* printf.h - generic printf implementation */
#ifndef _KERNEL_PRINTF_H
#define _KERNEL_PRINTF_H

#include <kernel/cdefs.h>

int generic_printf(void (*putchar)(char c), const char* restrict format, va_list parameters);

#endif
