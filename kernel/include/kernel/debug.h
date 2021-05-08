/* debug.h - debug functions */
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include <kernel/cdefs.h>

void debug_initialize(void);

int kdprintf(const char* __restrict, ...);

noreturn kabort(void);

noreturn _kassert_failed(const char *expr, const char *file, unsigned int line);

#define kassert(expr)	\
	if (!(expr))		\
		_kassert_failed(#expr, __FILE__, __LINE__);

#endif
