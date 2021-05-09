/*
	debug.h - debug functions

	These are guarenteed be usable in any state of the kernel, except:
		1. early boot stage, where all of kernel's executable might not be mapped yet,
		2. before init_debug() is called.
*/
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include <kernel/cdefs.h>

void init_debug(void);

int kdprintf(const char* __restrict, ...);
noreturn kpanic(const char *reason);
noreturn kabort(void);
noreturn _kassert_failed(const char *expr, const char *file, unsigned int line);
#define kassert(expr)	\
	if (!(expr))		\
		_kassert_failed(#expr, __FILE__, __LINE__);

#endif
